#include <vector>
#include <memory>
#include <iostream>
#include <map>
#include <systemd/sd-bus.h>
#include "bluetooth_device.h"

class BluetoothRegistry {
public:
    explicit BluetoothRegistry(sd_bus* busConnection)
        : bus(busConnection) {
        if (!bus) {
            std::cerr << "Invalid D-Bus connection provided." << std::endl;
        }
    }

    ~BluetoothRegistry() {
        // 외부에서 제공된 bus는 여기서 해제하지 않습니다.
    }

    void registerAllDevices();

    void addDevice(const char* devicePath);

    void removeDevice(const char* devicePath);

    void sendMessageAll(const void* report, size_t size);

private:
    sd_bus* bus;
    std::map<std::string, std::shared_ptr<BluetoothDevice>> devices;

    void readDevicePath(const char* devicePath, sd_bus_message* interfaces_and_properties);

    
};