#include <iostream>
#include <libusb-1.0/libusb.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <condition_variable>
#include "keyboard_util.h"
#include "global_vars.h"

extern void readThreadFunc(libusb_context* ctx);
extern void writeThreadFunc(libusb_context* ctx);
extern void loggerThreadFunc();

int main() {
    libusb_context *ctx;
    int r;

    r = libusb_init(&ctx);
    if (r < 0) {
        std::cerr << "Failed to initialize libusb." << std::endl;
        return 1;
    }

    auto currentTimestamp = std::chrono::high_resolution_clock::now();
    lastTimestamp = currentTimestamp;

    std::thread readThread(readThreadFunc, ctx);
    std::thread writeThread(writeThreadFunc, ctx);

    std::thread loggerThread(loggerThreadFunc);
    // 나머지 코드...

    // 프로그램 종료 직전에 이 코드 추가:
    logCondition.notify_one();
    loggerThread.join();

    readThread.join();
    writeThread.join();

    libusb_exit(ctx);
    return 0;
}
