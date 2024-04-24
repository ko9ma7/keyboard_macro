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
    
    readThread->waitForCompletion();  // 스레드가 완료될 때까지 기다림
    std::cout << "실행 종료: " << request->filename() << " 디버그 모드" << std::endl;

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
    std::cout<<"종료 요청\n";
    readThread->stopMacroReplay();
    response->set_message("매크로 재생 중단");
    return grpc::Status::OK;
}

grpc::Status InputServiceImpl::StartComplexReplay(grpc::ServerContext* context, const ComplexReplayRequest* request, StatusResponse* response) {

    std::cout<<"복잡한 요청 실행"<<request->repeatcount()<<'\n';

    std::vector<read_thread_ns::ReplayRequest> requests; // ReplayRequest 구조체의 벡터를 생성합니다.

    // ComplexReplayRequest로부터 ReplayRequest 벡터를 구성합니다.
    for (const auto& task : request->tasks()) {
        read_thread_ns::ReplayRequest replayRequest = {task.filename(), task.delayafter()};
        requests.push_back(replayRequest);
    }

    readThread->startComplexRequests(requests, request->repeatcount());

    std::cout << "실행 종료: "<< std::endl;
    return grpc::Status::OK;
}

grpc::Status InputServiceImpl::GetMacroDetail(grpc::ServerContext* context, const GetMacroDetailRequest* request, GetMacroDetailResponse* response) {
    // 파일에서 매크로 데이터 읽기
    std::string filename = request->filename();
    std::vector<KeyMacro::KeyEvent> events = readThread->readMacroFile(filename);
    
    for (const auto& event : events) {
        KeyEvent* proto_event = response->add_events(); // 프로토콜 버퍼의 KeyEvent 객체 생성
        proto_event->set_delay(event.delay); // delay 설정
        proto_event->set_data(event.data.data(), event.data.size()); // data 설정
    }

    return grpc::Status::OK;
}

grpc::Status InputServiceImpl::DeleteMacros(grpc::ServerContext* context, const DeleteMacrosRequest* request, StatusResponse* response) {
    std::vector<std::string> filenames(request->filenames().begin(), request->filenames().end());
    
    loggerThread->deleteMacros(filenames);
    
    response->set_message("파일 삭제 완료");
    return grpc::Status::OK;
}