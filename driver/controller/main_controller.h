#ifndef THREAD_CONTROLLER_H
#define THREAD_CONTROLLER_H

#include "../read_thread/read_thread.h"
#include "../logger_thread/logger_thread.h"
#include "../grpc_thread/grpc_thread.h"
#include <memory>

class ThreadController {
public:
    ThreadController() {
        
    }

    int RunThread();
    int RunBluetoothThread();
    void RunServer();

private:
    LoggerThread loggerThread;
    std::shared_ptr<ReadThread> readThread;
    InputServiceImpl grpcThread;

    void injectDependencies() {
        grpcThread.loggerThread = &loggerThread;
        loggerThread.readThread = readThread.get();
        grpcThread.readThread = readThread.get();
    }
};

#endif // THREAD_CONTROLLER_H
