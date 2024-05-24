// grpc_thread.h
#ifndef GRPC_THREAD_H
#define GRPC_THREAD_H

#include <grpcpp/grpcpp.h>
#include "grpc/restart_service.grpc.pb.h"

class RestartServiceImpl final : public Restart::Service {

public:
    grpc::Status RestartProcess(grpc::ServerContext* context, const RestartRequest* request,
                                RestartResponse* response) override;
    
    grpc::Status RequestUpdate(grpc::ServerContext* context, const UpdateRequest* request,
                               UpdateResponse* response) override;
};

#endif // GRPC_THREAD_H
