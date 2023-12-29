#ifndef THREAD_CONTROLLER_H
#define THREAD_CONTROLLER_H

#include "../read_thread/read_thread.h"
#include "../logger_thread/logger_thread.h"
#include "../grpc_thread/grpc_thread.h"

#define INJECT_DEPENDENCY(FIELD, OBJECT) \
    OBJECT.FIELD = &FIELD;

class ThreadController {
public:
    ThreadController() {
        injectDependencies();
    }

    int RunThread();
    void RunServer();

private:
    LoggerThread loggerThread;
    ReadThread readThread;
    InputServiceImpl grpcThread;

    void injectDependencies() {
        INJECT_DEPENDENCY(loggerThread, grpcThread);
        INJECT_DEPENDENCY(readThread, loggerThread);
        INJECT_DEPENDENCY(readThread, grpcThread);
    }
};

#endif // THREAD_CONTROLLER_H
