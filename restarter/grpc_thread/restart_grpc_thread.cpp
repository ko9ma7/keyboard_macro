// grpc_thread.cpp
#include <cstdlib>
#include <iostream>
#include <thread>
#include <stdio.h>
#include "restart_grpc_thread.h"

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

    system("sudo /home/ccxz84/stop.sh");

    auto progressCallback = [writer](int progress, const std::string& status_message) {
        UpdateResponse response;
        response.set_progress(progress);
        response.set_status_message(status_message);

        // gRPC 스트림이 스레드 안전하지 않기 때문에 동기화 필요
        static std::mutex writerMutex;
        std::lock_guard<std::mutex> lock(writerMutex);

        writer->Write(response); // UpdateResponse 객체의 참조를 전달
    };

    UpdaterThread updater(progressCallback);

    updater.runUpdate();

    return grpc::Status::OK;
}
