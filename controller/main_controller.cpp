#include <iostream>
#include <libusb-1.0/libusb.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <condition_variable>
#include "main_controller.h"
#include "../utils/keyboard_util.h"
#include "../read_thread/bluetooth_read_thread.h"

int ThreadController::RunThread() {
    libusb_context *ctx;
    int r;

    std::thread serverThread([this]() { RunServer(); });

    r = libusb_init(&ctx);
    if (r < 0) {
        std::cerr << "Failed to initialize libusb." << std::endl;
        return 1;
    }

    readThread = std::make_shared<ReadThread>();
    injectDependencies();

    readThread->init();

    std::thread readThreadInstance(&ReadThread::readThreadFunc, readThread.get(), ctx);
    std::thread loggerThreadInstance(&LoggerThread::loggerThreadFunc, &loggerThread);

    readThread->getLogCondition().notify_one();
    loggerThreadInstance.join();

    readThreadInstance.join();

    libusb_exit(ctx);
    return 0;
}

int ThreadController::RunBluetoothThread() {
    libusb_context *ctx;
    int r;

    std::thread serverThread([this]() { RunServer(); });

    r = libusb_init(&ctx);
    if (r < 0) {
        std::cerr << "Failed to initialize libusb." << std::endl;
        return 1;
    }

    readThread = std::make_shared<BluetoothReadThread>();
    injectDependencies();

    std::thread readBluetoothThreadInstance([this]() {
        readThread->init();
    });
    std::thread readThreadInstance(&BluetoothReadThread::readThreadFunc, readThread.get(), ctx);
    std::thread loggerThreadInstance(&LoggerThread::loggerThreadFunc, &loggerThread);

    readThread->getLogCondition().notify_one();
    loggerThreadInstance.join();

    readThreadInstance.join();
    readBluetoothThreadInstance.join();
    libusb_exit(ctx);
    return 0;
}

void ThreadController::RunServer() {
    std::string server_address("0.0.0.0:50051");
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&this->grpcThread);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}
