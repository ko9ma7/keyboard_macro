#include <iostream>
#include <libusb-1.0/libusb.h>
#include <thread>

extern void readThreadFunc(libusb_context* ctx);
extern void writeThreadFunc(libusb_context* ctx);

int main() {
    libusb_context *ctx;
    int r;

    r = libusb_init(&ctx);
    if (r < 0) {
        std::cerr << "Failed to initialize libusb." << std::endl;
        return 1;
    }

    std::thread readThread(readThreadFunc, ctx);
    std::thread writeThread(writeThreadFunc, ctx);

    readThread.join();
    writeThread.join();

    libusb_exit(ctx);
    return 0;
}
