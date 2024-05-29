#include <iostream>
#include <boost/asio.hpp>
#include <systemd/sd-bus.h>
#include <functional>
#include <memory>
#include <thread>
#include <future>
#include <string>
#include <optional>
#include <chrono>
#include "bluetooth_agent.h"
#include "bluetooth_registry.h"

using namespace boost;
using namespace std;
using boost::asio::io_context;
using boost::asio::steady_timer;

#define DBUS_PATH_PROFILE "/ruundii/btkb_profile"
#define DBUS_PATH_AGENT "/ruundii/btkb_agent"
#define ROOT_OBJECT "/org/bluez"
#define ADAPTER_OBJECT "/org/bluez/hci0"
#define ADAPTER_INTERFACE "org.bluez.Adapter1"
#define DEVICE_INTERFACE "org.bluez.Device1"
#define OBJECT_MANAGER_INTERFACE "org.freedesktop.DBus.ObjectManager"
#define DEVICE_NAME "Bluetooth Keyboard12"
#define UUID "00001124-0000-1000-8000-00805f9b34fb"

class BluetoothAdapter: public std::enable_shared_from_this<BluetoothAdapter> {
public:
    BluetoothAdapter(boost::asio::io_context& io, sd_bus* bus);
    virtual ~BluetoothAdapter();

    boost::asio::io_context& io;
    sd_bus *bus = nullptr;
    boost::asio::posix::stream_descriptor sd_bus_fd;
    bool agentPublished;
    std::shared_ptr<BluetoothAgent> agent;
    bool omProxyInitialised;
    bool initialisingAdapter;

    std::shared_ptr<BluetoothRegistry> registry;

    void init();

    bool bt_service_running();

    void wait_bt_service_run();

    void check_bt_service_running();

    bool adapterExists();

    void waitTillAdapterPresentThenInit();

    void waitTillAdapterPresentThenInitSync();

    void setup_dbus_proxy();

    void registerAgent();

    static int onInterfacesAdded(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);

    static int onInterfacesRemoved(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);

    static int handle_method_call(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);

    bool setPower(bool power);

    bool setAlias(const std::string& alias);

    bool setDiscoverable(bool discoverable);

    // void cancelPairing();


    // static const GDBusInterfaceVTable interface_vtable;

    static const sd_bus_vtable bluetooth_vtable[];

    bool createAgent();

    bool registerAgentInBluez();

    bool setDefaultAgent();

    void on_sd_bus_event(const boost::system::error_code& ec, std::size_t bytes_transferred);
    void sd_bus_io_handler();

};