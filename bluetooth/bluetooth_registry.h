#include <vector>
#include <memory>
#include <iostream>
#include <gio/gio.h>
#include "bluetooth_device.h"

class BluetoothRegistry {
public:
    // GDBusConnection 객체를 생성자에서 받습니다.
    explicit BluetoothRegistry(GDBusConnection* busConnection)
        : bus(busConnection) {
        if (!bus) {
            std::cerr << "Invalid D-Bus connection provided." << std::endl;
        }
    }

    ~BluetoothRegistry() {
        // 외부에서 제공된 bus는 여기서 해제하지 않습니다.
    }

    void registerAllDevices();

private:
    GDBusConnection* bus;
    std::vector<std::shared_ptr<BluetoothDevice>> devices;

    void addDevice(const gchar* devicePath, GVariant* interfaces_and_properties);
};
