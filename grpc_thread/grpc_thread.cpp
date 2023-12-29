// grpc_thread.cpp
#include "grpc_thread.h"
#include <iostream>
#include <stdio.h>
#include "../utils/type.h"

grpc::Status InputServiceImpl::StartRecording(grpc::ServerContext* context, const StartRequest* request,
                                              StatusResponse* response) {
    std::cout << "녹화 시작, 파일 이름: " << request->filename() << std::endl;
    this->loggerThread->startLogging(request->filename()); // 파일 이름으로 로깅 시작
    this->loggerThread->setRecording(true);
    response->set_message("녹화 시작");
    return grpc::Status::OK;
}


grpc::Status InputServiceImpl::StopRecording(grpc::ServerContext* context, const StopRequest* request,
                                             StatusResponse* response) {
    std::cout << "녹화 중지" << std::endl;
    // 로그 기록 중지
    this->loggerThread->setRecording(false);

    // 로그 파일 닫기
    this->loggerThread->stopLogging();

    response->set_message("녹화 중지");
    return grpc::Status::OK;
}

grpc::Status InputServiceImpl::StartReplay(grpc::ServerContext* context, const ReplayRequest* request,
                                           StatusResponse* response) {
    std::cout << "매크로 재생 시작: " << request->filename() << std::endl;

    readThread->startMacroReplay(request->filename(), nullptr);

    response->set_message("매크로 재생 시작");
    return grpc::Status::OK;
}

grpc::Status InputServiceImpl::ReplayMacroDebug(grpc::ServerContext* context, const ReplayRequest* request,
                                                 grpc::ServerWriter<MacroEvent>* writer) {
    std::cout << "매크로 재생 시작: " << request->filename() << " 디버그 모드" << std::endl;

    readThread->startMacroReplay(request->filename(), [writer](const std::string& eventDescription) {
        MacroEvent event;
        event.set_eventdescription(eventDescription);
        writer->Write(event);
    });

    return grpc::Status::OK;
}

grpc::Status InputServiceImpl::SaveMacro(grpc::ServerContext* context, const SaveMacroRequest* request, SaveMacroResponse* response) {
    std::vector<KeyMacro::KeyEvent> events;
    for (const auto& eventProto : request->events()) {
        KeyMacro::KeyEvent event;
        event.delay = eventProto.delay();
        event.data.assign(eventProto.data().begin(), eventProto.data().end());
        events.push_back(event);
    }

    loggerThread->saveMacroToFile(events, request->filename());
    response->set_success(true);
    return grpc::Status::OK;
}

grpc::Status InputServiceImpl::ListSaveFiles(grpc::ServerContext* context, const ListRequest* request,
                                             SaveFilesResponse* response) {
    std::string directory = "save";
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.path().extension() == ".sav") {
            response->add_filenames(entry.path().filename().string());
        }
    }
    return grpc::Status::OK;
}

grpc::Status InputServiceImpl::StopReplay(grpc::ServerContext* context, const StopReplayRequest* request,
                            StatusResponse* response) {
    readThread->stopMacroReplay();
    response->set_message("매크로 재생 중단");
    return grpc::Status::OK;
}
