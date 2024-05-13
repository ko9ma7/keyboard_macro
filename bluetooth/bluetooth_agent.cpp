#include "bluetooth_agent.h"

void BluetoothAgent::Release() {
    // 필요한 로직 구현
    std::cout << "Release called." << std::endl;
}

void BluetoothAgent::AuthorizeService(const std::string& device, const std::string& uuid) {
    std::cout << "AuthorizeService (" << device << ", " << uuid << ")" << std::endl;
    setTrusted(device);
}

void BluetoothAgent::RequestConfirmation(const std::string& device, unsigned int passkey) {
    std::cout << "RequestConfirmation (" << device << ", " << passkey << ")" << std::endl;
}

void BluetoothAgent::RequestAuthorization(const std::string& device) {
    std::cout << "RequestAuthorization (" << device << ")" << std::endl;
    setTrusted(device);
}

void BluetoothAgent::RequestPinCode(const std::string& device) {
    std::cout << "RequestPinCode (" << device << ")" << std::endl;
}

void BluetoothAgent::RequestPasskey(const std::string& device) {
    std::cout << "RequestPasskey (" << device << ")" << std::endl;
}

void BluetoothAgent::DisplayPinCode(const std::string& device, const std::string& pinCode) {
    std::cout << "DisplayPinCode (" << device << ", " << pinCode << ")" << std::endl;
}

void BluetoothAgent::Cancel() {
    std::cout << "Cancel called." << std::endl;
}

void BluetoothAgent::setTrusted(const std::string& devicePath) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message* m = nullptr;
    int ret;

    // Trusted 속성 설정
    ret = sd_bus_message_new_method_call(
        bus,
        &m,
        "org.bluez",
        devicePath.c_str(),
        "org.freedesktop.DBus.Properties",
        "Set"
    );
    if (ret < 0) {
        std::cerr << "Failed to add method call: " << strerror(-ret) << std::endl;
        return;
    }

    // 메시지에 파라미터 추가
    sd_bus_message_append(m, "ssv", "org.bluez.Device1", "Trusted", "b", 1);

    // 동기 호출 실행
    ret = sd_bus_call(bus, m, 0, &error, nullptr);
    if (ret < 0) {
        std::cerr << "Failed to set device as trusted: " << error.message << std::endl;
        sd_bus_error_free(&error);
    } else {
        std::cout << "Device " << devicePath << " set as trusted." << std::endl;
    }

    sd_bus_message_unref(m);
}

// void BluetoothAgent::request_confirmation_response(const std::string& device, const std::string& passKey, const bool confirmed) {
//     // 이 메서드의 구현을 완료해야 합니다.
// }
