#include "bluetooth_read_thread.h"

void BluetoothReadThread::startComplexRequests(const std::vector<read_thread_ns::ReplayRequest>& requests, int repeatCount) {
    std::cout<<"반복 횟수 "<<repeatCount<<'\n';
    for (int i = 0; i < repeatCount; ++i) {
        for (const auto& request : requests) {
            for (int j = 0; j < request.repeatCount; ++j) {
                startMacroReplay(request.filename);
                waitForCompletion();
                if (stopRequested) {
                    if (hidg_fd >= 0) {
                        unsigned char stopReport[] = {0, 0, 0, 0, 0, 0, 0, 0};
                        this->outputWrite(hidg_fd, stopReport, sizeof(stopReport));
                        std::cout<<"reset fd: "<<hidg_fd<<"\n";
                    }
                    break; // 종료 플래그가 설정되면 루프를 종료
                }
                std::this_thread::sleep_for(std::chrono::seconds(request.delayAfter));  // 지정된 시간만큼 지연
            }
        }
    }
}

// int BluetoothReadThread::outputWrite(int fd, const void *buf, size_t count) {
//     if (!adapter) {
//         std::cerr << "Adapter is null, cannot send message." << std::endl;
//         return 1;
//     }

//     if (!adapter->registry) {
//         std::cerr << "Registry is null, cannot send message." << std::endl;
//         return 1;
//     }

//     std::cout << "send message \n";

//     uint8_t releaseReport[] = {0xa1, 0x01, 0x00, 0x00, 0x1d, 0x0, 0x0, 0x0, 0x0, 0x0};
//     adapter->registry->sendMessageAll(releaseReport, sizeof(releaseReport));


//     uint8_t releaseReport1[] = {0xa1, 0x01, 0x00, 0x00, 0x00, 0x0, 0x0, 0x0, 0x0, 0x0};
//     adapter->registry->sendMessageAll(releaseReport1, sizeof(releaseReport1));
//     // uint8_t releaseReport[] = {0xa1, 0x01, 0x00, 0x00, 0x1d, 0x0, 0x0, 0x0, 0x0, 0x0};
//     // sendKeyPress(releaseReport, sizeof(releaseReport));
//     // sleep(1);
//     // uint8_t releaseReport1[] = {0xa1, 0x01, 0x00, 0x00, 0x00, 0x0, 0x0, 0x0, 0x0, 0x0};
//     // sendKeyPress(releaseReport1, sizeof(releaseReport1));

//     return 1;
// }


int BluetoothReadThread::outputWrite(int fd, const void *buf, size_t count) {
    if (!adapter) {
        std::cerr << "Adapter is null, cannot send message." << std::endl;
        return 1;
    }

    if (!adapter->registry) {
        std::cerr << "Registry is null, cannot send message." << std::endl;
        return 1;
    }

    size_t newBufSize = count + 2;
    uint8_t* newBuf = new uint8_t[newBufSize];

    newBuf[0] = 0xa1;
    newBuf[1] = 0x01;

    // 나머지 buf 내용을 새로운 버퍼에 복사
    std::memcpy(newBuf + 2, buf, count);

    // 새로운 버퍼를 사용하여 sendMessageAll 호출
    adapter->registry->sendMessageAll(newBuf, newBufSize);

    // 동적 할당된 메모리 해제
    delete[] newBuf;

    return count;
}

void BluetoothReadThread::init() {
     try {
        boost::asio::io_context io;

        std::cout<<"start bluetooth init\n";

        // D-Bus 시스템 버스에 연결
        sd_bus* bus = nullptr;
        if (sd_bus_open_system(&bus) < 0) {
            std::cerr << "Failed to open system bus." << std::endl;
            return ;
        }

        // BluetoothAdapter 인스턴스 생성
        adapter = std::make_shared<BluetoothAdapter>(io, bus);
        adapter->init();
        // io_context를 별도의 쓰레드에서 실행
        io.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return;
    }

    return;
}

int BluetoothReadThread::outputOpen(const char* path, int flags) {
    return 1;
}

int BluetoothReadThread::outputClose(int fd) {
    return 1;
}