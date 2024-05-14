#include <systemd/sd-bus.h>
#include "bluetooth_adapter.h"

const sd_bus_vtable BluetoothAdapter::bluetooth_vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("AuthorizeService", "os", "s", &BluetoothAdapter::handle_method_call, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("RequestConfirmation", "ou", "s", &BluetoothAdapter::handle_method_call, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("RequestAuthorization", "o", "", &BluetoothAdapter::handle_method_call, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("RequestPinCode", "o", "s", &BluetoothAdapter::handle_method_call, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("RequestPasskey", "o", "u", &BluetoothAdapter::handle_method_call, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("Cancel", "", "", &BluetoothAdapter::handle_method_call, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("Release", "", "", &BluetoothAdapter::handle_method_call, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END
};

void BluetoothAdapter::on_sd_bus_event(const boost::system::error_code& ec, std::size_t bytes_transferred) {
    if (!ec) {
        sd_bus_process(bus, nullptr);
        sd_bus_flush(bus);
        sd_bus_io_handler();
    } else {
        std::cerr << "Error on sd_bus event: " << ec.message() << std::endl;
    }
}

void BluetoothAdapter::sd_bus_io_handler() {
    sd_bus_fd.async_read_some(boost::asio::null_buffers(), 
        std::bind(&BluetoothAdapter::on_sd_bus_event, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

int BluetoothAdapter::handle_method_call(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    BluetoothAdapter *self = static_cast<BluetoothAdapter*>(userdata);
    const char *member = sd_bus_message_get_member(m);

    std::cout<<"handle_method\n";

    if (strcmp(member, "AuthorizeService") == 0) {
        const char *device, *uuid;
        if (sd_bus_message_read(m, "os", &device, &uuid) < 0) {
            sd_bus_reply_method_errorf(m, "org.freedesktop.DBus.Error.InvalidArgs", "Invalid parameters for AuthorizeService");
            return -1;
        }
        std::cout << "AuthorizeService called with device: " << device << " and UUID: " << uuid << std::endl;
        sd_bus_reply_method_return(m, "s", "Service Authorized");
    } 
    else if (strcmp(member, "RequestConfirmation") == 0) {
        const char *device;
        uint32_t passkey;
        if (sd_bus_message_read(m, "ou", &device, &passkey) < 0) {
            sd_bus_reply_method_errorf(m, "org.freedesktop.DBus.Error.InvalidArgs", "Invalid parameters for RequestConfirmation");
            return -1;
        }
        std::cout << "RequestConfirmation called with device: " << device << " and passkey: " << passkey << std::endl;
        sd_bus_reply_method_return(m, "s", "Confirmation Requested");
    } 
    else if (strcmp(member, "RequestAuthorization") == 0) {
        const char *device;
        if (sd_bus_message_read(m, "o", &device) < 0) {
            sd_bus_reply_method_errorf(m, "org.freedesktop.DBus.Error.InvalidArgs", "Invalid parameters for RequestAuthorization");
            return -1;
        }
        std::cout << "RequestAuthorization called for device: " << device << std::endl;
        sd_bus_reply_method_return(m, "s", "Authorization Requested");
    } 
    else if (strcmp(member, "RequestPinCode") == 0) {
        const char *device;
        if (sd_bus_message_read(m, "o", &device) < 0) {
            sd_bus_reply_method_errorf(m, "org.freedesktop.DBus.Error.InvalidArgs", "Invalid parameters for RequestPinCode");
            return -1;
        }
        std::cout << "RequestPinCode called for device: " << device << std::endl;
        sd_bus_reply_method_return(m, "s", "1234");  // Example pin code
    } 
    else if (strcmp(member, "RequestPasskey") == 0) {
        const char *device;
        if (sd_bus_message_read(m, "o", &device) < 0) {
            sd_bus_reply_method_errorf(m, "org.freedesktop.DBus.Error.InvalidArgs", "Invalid parameters for RequestPasskey");
            return -1;
        }
        std::cout << "RequestPasskey called for device: " << device << std::endl;
        sd_bus_reply_method_return(m, "u", 123456);  // Example passkey
    } 
    else if (strcmp(member, "Cancel") == 0) {
        std::cout << "Cancel called" << std::endl;
        sd_bus_reply_method_return(m, "s", "Cancelled");
    } 
    else if (strcmp(member, "Release") == 0) {
        std::cout << "Release called" << std::endl;
        sd_bus_reply_method_return(m, "s", "Released");
    } 
    else {
        sd_bus_reply_method_errorf(m, "org.freedesktop.DBus.Error.UnknownMethod", "Unknown method: %s", member);
        std::cerr << "Unknown method call: " << member << std::endl;
        return -1;
    }

    return 1;  // Indicates successful handling
}

BluetoothAdapter::BluetoothAdapter(boost::asio::io_context& io, sd_bus* bus)
: io(io), bus(bus), agentPublished(false), sd_bus_fd(io, sd_bus_get_fd(bus)), initialisingAdapter(false) {
    registry = std::make_shared<BluetoothRegistry>(bus);
}

BluetoothAdapter::~BluetoothAdapter() {

}

void BluetoothAdapter::init() {
    sd_bus_io_handler();
    wait_bt_service_run();
    setup_dbus_proxy();
    waitTillAdapterPresentThenInitSync();
}

bool BluetoothAdapter::bt_service_running() {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = nullptr;
    int ret = sd_bus_call_method(bus,
                                 "org.bluez",            // Service to contact
                                 "/",                    // Object path
                                 "org.freedesktop.DBus.ObjectManager", // Interface name
                                 "GetManagedObjects",    // Method name
                                 &error,                 // Pointer to initialize on error
                                 &m,                     // Response message on success
                                 "");                    // Input signature (no input arguments)

    if (ret < 0) {
        std::cerr << "Failed to call method: " << error.message << std::endl;
        sd_bus_error_free(&error);
        return false;
    }

    // If successful, there are objects managed by bluez which means Bluetooth service is running
    sd_bus_message_unref(m);
    return true;
}

void BluetoothAdapter::wait_bt_service_run() {
    auto self(shared_from_this()); // 클래스의 shared_ptr를 캡쳐하기 위해
    io.post([this, self]() { check_bt_service_running(); });
}

void BluetoothAdapter::check_bt_service_running() {
    if (!bt_service_running()) {
        std::cout << "No BT service. Waiting..." << std::endl;
        // 2초 후에 다시 확인
        auto self(shared_from_this()); // 클래스의 shared_ptr를 캡쳐하기 위해
        boost::asio::steady_timer timer(io, boost::asio::chrono::seconds(2));
        timer.async_wait([this, self](const boost::system::error_code& /*e*/) {
            check_bt_service_running();
        });
    }
}

bool BluetoothAdapter::adapterExists() {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    char *version = nullptr;  // const char*를 char*로 변경
    int ret;

    // BlueZ 어댑터 인터페이스에서 "Version" 속성을 가져오기 위한 호출
    ret = sd_bus_get_property_string(bus,
                                     "org.bluez",                // 서비스 이름
                                     ADAPTER_OBJECT,             // 오브젝트 경로
                                     ADAPTER_INTERFACE,          // 인터페이스 이름
                                     "Version",                  // 속성 이름
                                     &error,                     // 오류 처리
                                     &version);                  // 속성 값의 출력 매개변수

    if (ret < 0) {
        std::cerr << "Failed to read 'Version' property: " << error.message << std::endl;
        sd_bus_error_free(&error);
        return false;
    }

    // 버전 문자열을 비교하여 어댑터가 존재하는지 확인
    bool exists = (std::string(version) == "Hacked");
    free(version);  // version 문자열에 할당된 메모리를 해제
    sd_bus_error_free(&error); // 오류 객체 정리
    return exists;
}

void BluetoothAdapter::waitTillAdapterPresentThenInitSync() {
    if (initialisingAdapter) {
        return; // already initing
    }
    initialisingAdapter = true;
    std::cout<<"initing\n";
    auto future = std::async(std::launch::async, [this] { waitTillAdapterPresentThenInit(); });
}

void BluetoothAdapter::setup_dbus_proxy() {
    if (!omProxyInitialised) {
        int ret;
        ret = sd_bus_match_signal(bus, NULL, "org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "InterfacesAdded", &BluetoothAdapter::onInterfacesAdded, this);
        if (ret < 0) {
            std::cerr << "Failed to subscribe to InterfacesAdded signals: " << strerror(-ret) << std::endl;
            return;
        }

        ret = sd_bus_match_signal(bus, NULL, "org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "InterfacesRemoved", &BluetoothAdapter::onInterfacesRemoved, this);
        if (ret < 0) {
            std::cerr << "Failed to subscribe to InterfacesRemoved signals: " << strerror(-ret) << std::endl;
            return;
        }

        omProxyInitialised = true;
        std::cout << "Object Manager proxy initialised and signals subscribed." << std::endl;
    }
}

int BluetoothAdapter::onInterfacesAdded(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    BluetoothAdapter *self = static_cast<BluetoothAdapter*>(userdata);
    const char *path;
    int r;

    // Read the object path
    r = sd_bus_message_read(m, "o", &path);
    if (r < 0) {
        std::cerr << "Failed to read object path: " << strerror(-r) << std::endl;
        return r;
    }

    std::cout << "Interface added at path: " << path << std::endl;

    // Now iterate over the array of interfaces
    r = sd_bus_message_enter_container(m, 'a', "{sa{sv}}");
    if (r < 0) {
        std::cerr << "Failed to enter container for interfaces: " << strerror(-r) << std::endl;
        return r;
    }

    while ((r = sd_bus_message_enter_container(m, 'e', "sa{sv}")) > 0) {
        const char *interface_name;
        r = sd_bus_message_read(m, "s", &interface_name);
        if (r < 0) {
            std::cerr << "Failed to read interface name: " << strerror(-r) << std::endl;
            break;
        }

        std::cout << "Interface: " << interface_name << std::endl;

        // Enter the variant containing properties
        r = sd_bus_message_enter_container(m, 'a', "{sv}");
        if (r < 0) {
            std::cerr << "Failed to enter properties container: " << strerror(-r) << std::endl;
            break;
        }

        const char *property_name;
        while ((r = sd_bus_message_enter_container(m, 'e', "sv")) > 0) {
            r = sd_bus_message_read(m, "s", &property_name);
            if (r < 0) {
                std::cerr << "Failed to read property name: " << strerror(-r) << std::endl;
                break;
            }

            // Skipping actual property value reading for brevity
            sd_bus_message_skip(m, "v");

            sd_bus_message_exit_container(m);  // Exit the dict entry
        }

        sd_bus_message_exit_container(m);  // Exit the array of properties
        sd_bus_message_exit_container(m);  // Exit the dict entry
    }

    sd_bus_message_exit_container(m);  // Exit the array of interfaces

    self->registry->addDevice(path);

    return 1;
}

int BluetoothAdapter::onInterfacesRemoved(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    BluetoothAdapter *self = static_cast<BluetoothAdapter*>(userdata);
    const char *path;
    int r;

    r = sd_bus_message_read(m, "o", &path);
    if (r < 0) {
        std::cerr << "Failed to read object path: " << strerror(-r) << std::endl;
        return r;
    }

    std::cout << "Interface removed from path: " << path << std::endl;

    // Read the array of removed interfaces
    r = sd_bus_message_enter_container(m, 'a', "s");
    if (r < 0) {
        std::cerr << "Failed to enter container for interfaces: " << strerror(-r) << std::endl;
        return r;
    }

    const char *interface_name;
    while ((r = sd_bus_message_read(m, "s", &interface_name)) > 0) {
        std::cout << "Removed Interface: " << interface_name << std::endl;
    }

    sd_bus_message_exit_container(m);

    self->registry->removeDevice(path);

    return 1;
}

void BluetoothAdapter::waitTillAdapterPresentThenInit() {
    while (!adapterExists()) {
        std::cout << "No BT adapter. Waiting..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    registerAgent();

    setAlias(DEVICE_NAME);
    setDiscoverable(true);

    registry->registerAllDevices();
}

bool BluetoothAdapter::createAgent() {
    int r = sd_bus_add_object_vtable(
        bus,
        nullptr, // Pass nullptr if you don't want to track the slot
        "/org/bluez/agent",   // Object path
        "org.bluez.Agent1",   // Interface name
        bluetooth_vtable,
        this                  // User data
    );

    if (r < 0) {
        std::cerr << "Failed to add object vtable: " << strerror(-r) << std::endl;
        return false;
    }

    agent = std::make_shared<BluetoothAgent>(bus);
    return true;
}

bool BluetoothAdapter::registerAgentInBluez() {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = nullptr;
    int r;

    // 에이전트 등록
    r = sd_bus_call_method(bus,
                           "org.bluez",
                           "/org/bluez",
                           "org.bluez.AgentManager1",
                           "RegisterAgent",
                           &error,
                           &m,
                           "os",
                           "/org/bluez/agent",
                           "KeyboardDisplay");

    if (r < 0) {
        std::cerr << "Failed to register agent: " << error.message << std::endl;
        sd_bus_error_free(&error);
        if (m) sd_bus_message_unref(m);
        return false;
    }

    sd_bus_message_unref(m);
    sd_bus_error_free(&error);
    std::cout << "Agent registered successfully." << std::endl;
    return true;
}

bool BluetoothAdapter::setDefaultAgent() {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = nullptr;
    int r;

    // 기본 에이전트로 설정
    r = sd_bus_call_method(bus,
                           "org.bluez",
                           "/org/bluez",
                           "org.bluez.AgentManager1",
                           "RequestDefaultAgent",
                           &error,
                           &m,
                           "o",
                           "/org/bluez/agent");

    if (r < 0) {
        std::cerr << "Failed to set default agent: " << error.message << std::endl;
        sd_bus_error_free(&error);
        if (m) sd_bus_message_unref(m);
        return false;
    }

    sd_bus_message_unref(m);
    sd_bus_error_free(&error);
    std::cout << "Default agent set successfully." << std::endl;
    return true;
}

void BluetoothAdapter::registerAgent() {
    if (!agentPublished) {
        if (!createAgent()) {
            std::cerr << "Failed to create agent." << std::endl;
            return;
        }
        if (!registerAgentInBluez()) {
            std::cerr << "Failed to register agent in Bluez." << std::endl;
            return;
        }
        if (!setDefaultAgent()) {
            std::cerr << "Failed to set default agent." << std::endl;
            return;
        }
        agentPublished = true;
        std::cout << "Agent registered and set as default successfully." << std::endl;
    }
}

bool BluetoothAdapter::setPower(bool power) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    int r = sd_bus_call_method(bus,
                               "org.bluez",
                               ADAPTER_OBJECT,
                               "org.freedesktop.DBus.Properties",
                               "Set",
                               &error,
                               NULL,
                               "ssv",
                               "org.bluez.Adapter1",
                               "Powered",
                               "b",
                               power);

    if (r < 0) {
        std::cerr << "Failed to set power: " << error.message << std::endl;
        sd_bus_error_free(&error);
        return false;
    }

    std::cout << "Power set to " << (power ? "on" : "off") << std::endl;
    return true;
}

bool BluetoothAdapter::setAlias(const std::string& alias) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    int r = sd_bus_call_method(bus,
                               "org.bluez",
                               ADAPTER_OBJECT,
                               "org.freedesktop.DBus.Properties",
                               "Set",
                               &error,
                               NULL,
                               "ssv",
                               "org.bluez.Adapter1",
                               "Alias",
                               "s",
                               alias.c_str());

    if (r < 0) {
        std::cerr << "Failed to set alias: " << error.message << std::endl;
        sd_bus_error_free(&error);
        return false;
    }

    std::cout << "Alias set to " << alias << std::endl;
    return true;
}

bool BluetoothAdapter::setDiscoverable(bool discoverable) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    int r = sd_bus_call_method(bus,
                               "org.bluez",
                               ADAPTER_OBJECT,
                               "org.freedesktop.DBus.Properties",
                               "Set",
                               &error,
                               NULL,
                               "ssv",
                               "org.bluez.Adapter1",
                               "Discoverable",
                               "b",
                               discoverable);

    if (r < 0) {
        std::cerr << "Failed to set discoverable: " << error.message << std::endl;
        sd_bus_error_free(&error);
        return false;
    }

    std::cout << "Discoverable set to " << (discoverable ? "true" : "false") << std::endl;
    return true;
}