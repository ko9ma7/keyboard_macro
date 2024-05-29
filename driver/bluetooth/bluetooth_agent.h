#include <string>
#include <iostream>
#include <systemd/sd-bus.h>

class BluetoothAgent {
public:
    BluetoothAgent(sd_bus* bus) : bus(bus){}
    ~BluetoothAgent() {}

    void AuthorizeService(const std::string& device, const std::string& uuid);
    void RequestConfirmation(const std::string& device, unsigned int passkey);
    void RequestAuthorization(const std::string& device);
    void RequestPinCode(const std::string& device);
    void RequestPasskey(const std::string& device);
    void DisplayPinCode(const std::string& device, const std::string& pinCode);
    void Cancel();
    void setTrusted(const std::string& devicePath);

    void Release(); // 확실히 반환 타입이 void인지 확인

private:
    // GDBusConnection* connection; // 클래스 내부에서 DBus 연결을 위한 포인터
    sd_bus* bus;
};
