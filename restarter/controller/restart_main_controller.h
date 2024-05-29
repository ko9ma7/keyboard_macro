#ifndef THREAD_CONTROLLER_H
#define THREAD_CONTROLLER_H

#include "../grpc_thread/restart_grpc_thread.h"

class RestartThreadController {
public:
    void RunServer();

private:
    RestartServiceImpl grpcThread;
};

#endif // THREAD_CONTROLLER_H
