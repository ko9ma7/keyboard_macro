#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>

class BluetoothDevice {
private:
    int control_socket;
    int interrupt_socket;
    std::string control_socket_path;
    std::string interrupt_socket_path;
    bool sockets_connected = false;

public:
    BluetoothDevice(const std::string& ctrlPath, const std::string& intrPath) 
        : control_socket_path(ctrlPath), interrupt_socket_path(intrPath) {}

    void connectSockets();

    int setNonBlocking(int sock);

    void sendKeyPress(uint8_t* report);

    void releaseKeysAndCloseSockets();

    void disconnectSockets();

    ~BluetoothDevice() {
        releaseKeysAndCloseSockets();
    }
};
