#include <iostream>
#include <libusb-1.0/libusb.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <condition_variable>
#include "utils/keyboard_util.h"
#include "read_thread/read_thread.h"
#include "logger_thread/logger_thread.h"
#include "grpc_thread/grpc_thread.h"

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    InputServiceImpl service; 
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

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
    std::cout<<"sibal\n";

    // 나머지 코드...

    // 프로그램 종료 직전에 이 코드 추가:
    readThread.getLogCondition().notify_one();
    loggerThreadInstance.join();

    readThreadInstance.join();

    libusb_exit(ctx);
    return 0;
}