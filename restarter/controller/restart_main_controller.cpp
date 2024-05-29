#include <iostream>
#include "restart_main_controller.h"

void RestartThreadController::RunServer() {
    std::string server_address("0.0.0.0:50052");
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&this->grpcThread);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}
