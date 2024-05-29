#include "bluetooth_registry.h"

void BluetoothRegistry::registerAllDevices() {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message* m = nullptr;
    int r;

    // Get the managed objects from BlueZ
    r = sd_bus_call_method(bus,
                           "org.bluez",
                           "/",
                           "org.freedesktop.DBus.ObjectManager",
                           "GetManagedObjects",
                           &error,
                           &m,
                           "");
    if (r < 0) {
        std::cerr << "Failed to call GetManagedObjects: " << error.message << std::endl;
        sd_bus_error_free(&error);
        return;
    }

    const char* path;
    sd_bus_message_enter_container(m, 'a', "{oa{sa{sv}}}");
    while (sd_bus_message_enter_container(m, 'e', "oa{sa{sv}}") > 0) {
        sd_bus_message_read(m, "o", &path);
        std::cout << "Register device: " << path << std::endl;
        readDevicePath(path, m);
        addDevice(path);
        sd_bus_message_exit_container(m); // Exit 'e'
    }
    sd_bus_message_exit_container(m); // Exit 'a'

    sd_bus_message_unref(m);
}

void BluetoothRegistry::readDevicePath(const char* devicePath, sd_bus_message* interfaces_and_properties) {
    const char* interface_name;
    const char* property_name;

    std::map<std::string, std::string> properties;

    sd_bus_message_enter_container(interfaces_and_properties, 'a', "{sa{sv}}");
    while (sd_bus_message_enter_container(interfaces_and_properties, 'e', "sa{sv}") > 0) {
        sd_bus_message_read(interfaces_and_properties, "s", &interface_name);
        sd_bus_message_enter_container(interfaces_and_properties, 'a', "{sv}");
        while (sd_bus_message_enter_container(interfaces_and_properties, 'e', "sv") > 0) {
            sd_bus_message_read(interfaces_and_properties, "s", &property_name);
            const char* value_type = sd_bus_message_get_signature(interfaces_and_properties, true);

            if (strcmp(value_type, "s") == 0) {
                const char* value;
                sd_bus_message_enter_container(interfaces_and_properties, 'v', "s");
                sd_bus_message_read(interfaces_and_properties, "s", &value);
                properties[property_name] = value;
                std::cout << "Found property: " << property_name << " = " << value << std::endl;
                sd_bus_message_exit_container(interfaces_and_properties); // Exit 'v'
            } else {
                sd_bus_message_skip(interfaces_and_properties, "v");
            }
            sd_bus_message_exit_container(interfaces_and_properties); // Exit 'e'
        }
        sd_bus_message_exit_container(interfaces_and_properties); // Exit 'a'
        sd_bus_message_exit_container(interfaces_and_properties); // Exit 'e'
    }
    sd_bus_message_exit_container(interfaces_and_properties); // Exit 'a'
}

void BluetoothRegistry::addDevice(const char* devicePath) {
    if (devices.find(devicePath) != devices.end()) {
        std::cerr << "Device already registered: " << devicePath << std::endl;
        return;
    }

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message* m = nullptr;
    int r;

    // Get the SocketPathCtrl property
    r = sd_bus_get_property(bus,
                            "org.bluez",
                            devicePath,
                            "org.bluez.InputHost1",
                            "SocketPathCtrl",
                            &error,
                            &m,
                            "s");

    if (r < 0) {
        std::cerr << "Failed to get property 'SocketPathCtrl': " << error.message << std::endl;
        sd_bus_error_free(&error);
        if (m) sd_bus_message_unref(m);
        return;
    }

    const char* ctrl_path;
    sd_bus_message_read(m, "s", &ctrl_path);
    sd_bus_message_unref(m);

    // Get the SocketPathIntr property
    r = sd_bus_get_property(bus,
                            "org.bluez",
                            devicePath,
                            "org.bluez.InputHost1",
                            "SocketPathIntr",
                            &error,
                            &m,
                            "s");

    if (r < 0) {
        std::cerr << "Failed to get property 'SocketPathIntr': " << error.message << std::endl;
        sd_bus_error_free(&error);
        if (m) sd_bus_message_unref(m);
        return;
    }

    const char* intr_path;
    sd_bus_message_read(m, "s", &intr_path);
    sd_bus_message_unref(m);

    auto device = std::make_shared<BluetoothDevice>(ctrl_path, intr_path);
    devices[devicePath] = device;
    std::cout << "Device added: " << devicePath << " with control path: " << ctrl_path << " and interrupt path: " << intr_path << std::endl;
}

void BluetoothRegistry::sendMessageAll(const void* report, size_t size) {
    for (const auto& [devicePath, device] : devices) {
        device->sendKeyPress(report, size);
    }
}

void BluetoothRegistry::removeDevice(const char* devicePath) {
    auto it = devices.find(devicePath);
    if (it != devices.end()) {
        devices.erase(it);
        std::cout << "Device removed: " << devicePath << std::endl;
    } else {
        std::cerr << "Device not found: " << devicePath << std::endl;
    }
}


// int main() {
//     sd_bus* bus = nullptr;
//     int r = sd_bus_open_system(&bus);
//     if (r < 0) {
//         std::cerr << "Failed to connect to system bus: " << strerror(-r) << std::endl;
//         return 1;
//     }

//     BluetoothRegistry registry(bus);

//     registry.registerAllDevices();

//     sd_bus_unref(bus);
//     return 0;
// }