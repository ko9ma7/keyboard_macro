// grpc_thread.cpp
#include "grpc_thread.h"
#include <iostream>

grpc::Status InputServiceImpl::StartRecording(grpc::ServerContext* context, const StartRequest* request,
                                              StatusResponse* response) {
    // 녹화 시작 로직
    std::cout << "녹화 시작" << std::endl;
    response->set_message("녹화 시작");
    return grpc::Status::OK;
}

grpc::Status InputServiceImpl::StopRecording(grpc::ServerContext* context, const StopRequest* request,
                                             StatusResponse* response) {
    // 녹화 종료 로직
    std::cout << "녹화 종료" << std::endl;
    response->set_message("녹화 종료");
    return grpc::Status::OK;
}
