// grpc_thread.cpp
#include <iostream>
#include <stdio.h>
#include "restart_grpc_thread.h"

grpc::Status RestartServiceImpl::RestartProcess(grpc::ServerContext* context, const RestartRequest* request,
                                              RestartResponse* response) {
    std::cout << "재시작 요청\n";

    return grpc::Status::OK;
}