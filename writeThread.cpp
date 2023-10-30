#include <iostream>
#include <libusb-1.0/libusb.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <functional>
#include <map>
#include "keyboard_util.h"
#include <mutex>
#include <condition_variable>
#include "global_vars.h"

#define HIDG_WRITE_PATH "/dev/hidg1"

enum Action {
    START_RECORDING,
    STOP_RECORDING,
    // 여기에 필요한 다른 작업을 추가할 수 있습니다.
};

// 녹화 시작 함수
void startRecording() {
    std::lock_guard<std::mutex> lock(recordingMutex);
    isRecording = true;
    isStartingToRecord = true;
    recordingCondition.notify_all();
    std::cout << "녹화 시작" << std::endl;
}

// 녹화 종료 함수
void stopRecording() {
    std::lock_guard<std::mutex> lock(recordingMutex);
    isRecording = false;
    recordingCondition.notify_all();
    std::cout << "녹화 종료1" << std::endl;
}


void writeThreadFunc(libusb_context* ctx) {
    int hidg_fd_read = open(HIDG_WRITE_PATH, O_RDONLY);
    if (hidg_fd_read < 0) {
        std::cerr << "Failed to open " << HIDG_WRITE_PATH << std::endl;
        return;
    }

    std::map<std::string, std::function<void()>> actionMap = {
        {"0010000000000000", startRecording},
        {"0020000000000000", stopRecording}
    };

    unsigned char data[8];

    while (true) {
        memset(data, 0, sizeof(data));
        // HID 장치에서 데이터 읽기
        ssize_t bytesRead = read(hidg_fd_read, data, sizeof(data));
        if (bytesRead < 0) {
            std::cerr << "Error reading from " << HIDG_WRITE_PATH << std::endl;
            break;
        }

        std::string dataStr;
        for (ssize_t i = 0; i < bytesRead; ++i) {
            char buffer[3];
            snprintf(buffer, sizeof(buffer), "%02X", data[i]);
            dataStr += buffer;
        }


        // 여기서는 예시로 데이터를 화면에 출력합니다.
        std::cout <<"size: "<<bytesRead<< " Received data: ";
        for (ssize_t i = 0; i < bytesRead; ++i) {
            std::cout << std::hex << std::uppercase << (int)data[i] << " ";
        }
        std::cout << std::endl;
        std::cout<<dataStr<<'\n';

        if (actionMap.find(dataStr) != actionMap.end()) {
            actionMap[dataStr](); // 매핑된 함수 바로 실행
        }
    }

    close(hidg_fd_read);
}