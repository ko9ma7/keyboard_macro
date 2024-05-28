// grpc_thread.cpp
#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include "restart_grpc_thread.h"
#include "../updater/updater_thread.h"

grpc::Status RestartServiceImpl::RestartProcess(grpc::ServerContext* context, const RestartRequest* request,
                                              RestartResponse* response) {
    std::cout << "재시작 요청\n";

    // 스크립트 실행
    system("sudo /home/ccxz84/restart.sh");

    std::cout << "재시작 완료\n";

    // 응답 전송
    response->set_message("Restart initiated.");

    return grpc::Status::OK;
}

grpc::Status RestartServiceImpl::RequestUpdate(grpc::ServerContext* context, const UpdateRequest* request,
                                               grpc::ServerWriter<UpdateResponse>* writer) {
    std::cout << "업데이트 요청\n";

    UpdaterThread updater(writer);
    updater.runUpdate();

    return grpc::Status::OK;
}
