// grpc_thread.cpp
#include "grpc_thread.h"
#include <iostream>
#include <stdio.h>

grpc::Status InputServiceImpl::StartRecording(grpc::ServerContext* context, const StartRequest* request,
                                              StatusResponse* response) {
    // 녹화 시작 로직
    std::cout << "녹화 시작" << std::endl;
    this->loggerThread->setRecording(true);
    response->set_message("녹화 시작");
    return grpc::Status::OK;
}

grpc::Status InputServiceImpl::StopRecording(grpc::ServerContext* context, const StopRequest* request,
                                             StatusResponse* response) {
    std::cout << "녹화 종료 및 파일 저장: " << request->filename() << std::endl;

    // 녹화 종료
    this->loggerThread->setRecording(false);

    // 파일에 로그 데이터 저장
    bool success = this->loggerThread->renameLogFile(request->filename());

    if (success) {
        response->set_message("녹화 종료 및 파일 저장 성공");
    } else {
        response->set_message("녹화 종료 실패 또는 파일 저장 실패");
    }

    return grpc::Status::OK;
}
