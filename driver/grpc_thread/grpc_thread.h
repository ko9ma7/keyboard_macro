// grpc_thread.h
#ifndef GRPC_THREAD_H
#define GRPC_THREAD_H

#include <grpcpp/grpcpp.h>
#include "grpc/input_service.grpc.pb.h"
#include "../logger_thread/logger_thread.h"
#include "../read_thread/read_thread.h"

class InputServiceImpl final : public Input::Service {

public:
    LoggerThread* loggerThread;

    ReadThread* readThread;

    grpc::Status StartRecording(grpc::ServerContext* context, const StartRequest* request,
                                StatusResponse* response) override;
    grpc::Status StopRecording(grpc::ServerContext* context, const StopRequest* request,
                               StatusResponse* response) override;
    grpc::Status StartReplay(grpc::ServerContext* context, const ReplayRequest* request,
        StatusResponse* response) override;
    grpc::Status ReplayMacroDebug(grpc::ServerContext* context, const ReplayRequest* request,
        grpc::ServerWriter<MacroEvent>* writer) override;
    grpc::Status ListSaveFiles(grpc::ServerContext* context, const ListRequest* request,
        SaveFilesResponse* response) override;
    grpc::Status StopReplay(grpc::ServerContext* context, const StopReplayRequest* request,
        StatusResponse* response) override;
    grpc::Status SaveMacro(grpc::ServerContext* context, const SaveMacroRequest* request, 
        SaveMacroResponse* response) override;
    grpc::Status GetMacroDetail(grpc::ServerContext* context, const GetMacroDetailRequest* request,
        GetMacroDetailResponse* response) override;
    grpc::Status DeleteMacros(grpc::ServerContext* context, const DeleteMacrosRequest* request,
        StatusResponse* response) override;
    grpc::Status StartComplexReplay(grpc::ServerContext* context, const ComplexReplayRequest* request,
        StatusResponse* response) override;

    grpc::Status ImportProfile(grpc::ServerContext* context, const ImportProfileRequest* request, StatusResponse* response) override;
    grpc::Status ExportProfile(grpc::ServerContext* context, const ExportProfileRequest* request, ExportProfileResponse* response) override;
};

#endif // GRPC_THREAD_H
