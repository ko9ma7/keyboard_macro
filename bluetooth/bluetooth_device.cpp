#include "bluetooth_device.h"

void BluetoothDevice::connectSockets() {
    control_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (control_socket == -1) {
        std::cerr << "Failed to create control socket: " << strerror(errno) << '\n';
        return;
    }

    struct sockaddr_un ctrl_addr;
    memset(&ctrl_addr, 0, sizeof(ctrl_addr));
    ctrl_addr.sun_family = AF_UNIX;
    strncpy(ctrl_addr.sun_path, control_socket_path.c_str(), sizeof(ctrl_addr.sun_path) - 1);

    if (connect(control_socket, (struct sockaddr*)&ctrl_addr, sizeof(ctrl_addr)) == -1) {
        std::cerr << "Failed to connect control socket: " << strerror(errno) << '\n';
        close(control_socket);
        return;
    }

    interrupt_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (interrupt_socket == -1) {
        std::cerr << "Failed to create interrupt socket: " << strerror(errno) << '\n';
        close(control_socket); // Close the previously opened control socket
        return;
    }

    struct sockaddr_un intr_addr;
    memset(&intr_addr, 0, sizeof(intr_addr));
    intr_addr.sun_family = AF_UNIX;
    strncpy(intr_addr.sun_path, interrupt_socket_path.c_str(), sizeof(intr_addr.sun_path) - 1);

    if (connect(interrupt_socket, (struct sockaddr*)&intr_addr, sizeof(intr_addr)) == -1) {
        std::cerr << "Failed to connect interrupt socket: " << strerror(errno) << '\n';
        close(control_socket);
        close(interrupt_socket);
        return;
    }

    setNonBlocking(interrupt_socket);

    sockets_connected = true;
    std::cout << "Sockets connected successfully." << '\n';
}

int BluetoothDevice::setNonBlocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Failed to get socket flags" << '\n';
        return -1;
    }
    
    flags |= O_NONBLOCK;
    if (fcntl(sock, F_SETFL, flags) == -1) {
        std::cerr << "Failed to set non-blocking socket" << '\n';
        return -1;
    }
    return 0;
}

void BluetoothDevice::sendKeyPress(const void* report, size_t size) {
    if (report == nullptr) {
        return;
    }
    if (!sockets_connected) {
        std::cerr << "Sockets not connected. Cannot send key press." << '\n';
        return;
    }

    if (send(interrupt_socket, report, size, 0) == -1) {
        std::cerr << "Failed to send key press: " << strerror(errno) <<'\n';
    }
}

void BluetoothDevice::releaseKeysAndCloseSockets() {
    // 모든 키 릴리즈 메시지를 생성합니다.
    // 예를 들어, HID 키보드의 경우 모든 키를 릴리즈하는 리포트를 생성할 수 있습니다.
    uint8_t releaseReport[10] = {0xA1, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  // 모든 키 릴리즈

    // 인터럽트 소켓에 리포트를 전송합니다.
    if (send(interrupt_socket, releaseReport, sizeof(releaseReport), 0) == -1) {
        std::cerr << "Failed to send release keys: " << strerror(errno) << '\n';
    }

    // 소켓 연결을 닫습니다.
    disconnectSockets();
}

void BluetoothDevice::disconnectSockets() {
    if (control_socket) {
        if (close(control_socket) == -1) {
            std::cerr << "Failed to close control socket: " << strerror(errno) <<'\n';
        } else {
            std::cout << "Control socket closed successfully." <<'\n';
        }
        control_socket = 0;  // 소켓을 초기화
    }

    if (interrupt_socket) {
        if (close(interrupt_socket) == -1) {
            std::cerr << "Failed to close interrupt socket: " << strerror(errno) <<'\n';
        } else {
            std::cout << "Interrupt socket closed successfully." <<'\n';
        }
        interrupt_socket = 0;  // 소켓을 초기화
    }

    sockets_connected = false;  // 연결 상태를 업데이트
}

BluetoothDevice::BluetoothDevice(const std::string& ctrlPath, const std::string& intrPath) 
: control_socket_path(ctrlPath), interrupt_socket_path(intrPath) {
    connectSockets();
}
