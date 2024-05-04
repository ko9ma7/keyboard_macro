#include <string>
#include <thread>
#include <time.h>
#include <chrono>
#include <iostream>
#include <sstream>
#include <cstring>
#include <vector>
#include <fstream>    // 파일 스트림을 위해
#include <unistd.h>   // POSIX 운영체제 API
#include <fcntl.h>    // 파일 컨트롤 정의
#include <ctime>      // 시간 함수
#include <cerrno>     // 에러 번호 정의
#include "./utils/keyboard_util.h"
#include "./utils/type.h"

void _replayMacro(const std::string& logFilename) {
    std::string fullPath = "save/" + logFilename;  // 전체 파일 경로 생성
    std::ifstream logFile(fullPath);  // 전체 경로를 사용하여 파일 열기
    std::string line;

    if (!logFile.is_open()) {
        std::cerr << "Error opening log file." << std::endl;
        return;
    }

    // Load all events into memory
    std::vector<KeyMacro::KeyEvent> events;
    while (std::getline(logFile, line)) {
        size_t posStart = line.find("Time diff: ") + 11;
        size_t posEnd = line.find("ns, Data: ");
        std::string timeDiffStr = line.substr(posStart, posEnd - posStart);
        uint64_t timeDiff = std::stoull(timeDiffStr, nullptr, 16);

        std::vector<unsigned char> buffer;
        std::string dataStr = line.substr(posEnd + 10);
        std::istringstream dataStream(dataStr);
        while (dataStream) {
            char byteStr[3] = {0};
            dataStream.read(byteStr, 2);
            if (dataStream.gcount() != 2) break;
            buffer.push_back(static_cast<unsigned char>(std::stoi(byteStr, nullptr, 16)));
            if (dataStream.peek() == ' ') dataStream.ignore();
        }

        events.push_back({timeDiff, buffer});
    }
    logFile.close();

    // Open HID device
    int hidg_fd = open(HIDG_MACRO_PATH, O_RDWR);
    if (hidg_fd < 0) {
        perror("Failed to open " HIDG_MACRO_PATH);
        return;
    }

    // Initial 1 second delay
    std::this_thread::sleep_for(std::chrono::seconds(1));

    struct timespec currentTime;
    clock_gettime(CLOCK_MONOTONIC, &currentTime);

    for (const auto& event : events) {

        struct timespec targetTime;
        
        targetTime.tv_sec = currentTime.tv_sec + (event.delay / 1000000000);
        targetTime.tv_nsec = currentTime.tv_nsec + (event.delay % 1000000000);

        // int randomDelay = distribution(generator);  // 난수 생성
        // targetTime.tv_nsec += randomDelay;  // 타겟 시간에 난수 추가

        // timespec 구조체 정규화
        if (targetTime.tv_nsec >= 1000000000) {
            targetTime.tv_sec += 1;
            targetTime.tv_nsec -= 1000000000;
        }

        struct timespec remaining;
        while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &targetTime, &remaining) == EINTR) {
            targetTime = remaining;
        }   
        
        write(hidg_fd, &event.data[0], event.data.size());
    }

    // HID device open check
    if (hidg_fd < 0) {
        perror("Failed to open " HIDG_READ_PATH " before replaying the events.");
        return;
    }

    // Close HID device
    close(hidg_fd);

    return;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <logFilename>" << std::endl;
        return 1;
    }

    _replayMacro(argv[1]);
    return 0;
}
