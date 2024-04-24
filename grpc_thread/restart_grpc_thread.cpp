// grpc_thread.cpp
#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include "restart_grpc_thread.h"

grpc::Status RestartServiceImpl::RestartProcess(grpc::ServerContext* context, const RestartRequest* request,
                                              RestartResponse* response) {
    std::cout << "재시작 요청\n";

    // 스크립트 실행
    system("sudo /home/ccxz84/restart.sh");

    // 응답 전송
    response->set_message("Restart initiated.");

    return grpc::Status::OK;
}