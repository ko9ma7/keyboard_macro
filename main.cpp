#include <iostream>
#include <libusb-1.0/libusb.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <condition_variable>
#include "keyboard_util.h"
#include "read_thread.h"
#include "logger_thread.h"

extern void writeThreadFunc(libusb_context* ctx);

int main() {
    ReadThread readThread;
    LoggerThread loggerThread;

    libusb_context *ctx;
    int r;

    r = libusb_init(&ctx);
    if (r < 0) {
        std::cerr << "Failed to initialize libusb." << std::endl;
        return 1;
    }

    auto currentTimestamp = std::chrono::high_resolution_clock::now();
    readThread.setLastTimestamp(currentTimestamp);

    loggerThread.setLogQueue(readThread.getLogQueue());
    loggerThread.setLogCondition(readThread.getLogCondition());
    loggerThread.setLogMutex(readThread.getLogMutex());
    loggerThread.setIsRecording(&readThread.isRecording);

    std::thread readThreadInstance(&ReadThread::readThreadFunc, &readThread, ctx);
    std::thread loggerThreadInstance(&LoggerThread::loggerThreadFunc, &loggerThread);

    // 나머지 코드...

    // 프로그램 종료 직전에 이 코드 추가:
    readThread.getLogCondition().notify_one();
    loggerThreadInstance.join();

    readThreadInstance.join();

    libusb_exit(ctx);
    return 0;
}