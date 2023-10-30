#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <sstream>
#include <time.h>

#define HIDG_READ_PATH "/dev/hidg0"

struct KeyEvent {
    uint64_t delay;
    std::vector<unsigned char> data;
};

int main() {
    std::ifstream logFile("keyboard_logs.txt");
    std::string line;

    if (!logFile.is_open()) {
        std::cerr << "Error opening log file." << std::endl;
        return 1;
    }

    // Load all events into memory
    std::vector<KeyEvent> events;
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
    int hidg_fd = open(HIDG_READ_PATH, O_RDWR);
    if (hidg_fd < 0) {
        perror("Failed to open " HIDG_READ_PATH);
        return 1;
    }

    // Initial 1 second delay
    std::this_thread::sleep_for(std::chrono::seconds(1));

    struct timespec req, rem; // 슬립을 위한 timespec 구조체

    // Replay events
    for (const auto& event : events) {
        req.tv_sec = event.delay / 1000000000;  // 초 단위
        req.tv_nsec = event.delay % 1000000000; // 나노초 단위

        while (clock_nanosleep(CLOCK_MONOTONIC, 0, &req, &rem) == EINTR) {
            // EINTR이 반환될 경우 중단된 슬립을 계속하기 위해 rem을 req에 대입
            req = rem;
        }

        ssize_t bytes_written = write(hidg_fd, &event.data[0], event.data.size());

        // Write error check
        if (bytes_written == -1) {
            std::cerr << "Failed to write to " HIDG_READ_PATH ". Error: " << strerror(errno) << std::endl;
            continue;
        } else if (bytes_written != static_cast<ssize_t>(event.data.size())) {
            std::cerr << "Partial write to " HIDG_READ_PATH ". Only " << bytes_written << " out of " << event.data.size() << " bytes were written." << std::endl;
        }
    }

    // HID device open check
    if (hidg_fd < 0) {
        perror("Failed to open " HIDG_READ_PATH " before replaying the events.");
        return 1;
    }

    // Close HID device
    close(hidg_fd);

    return 0;
}
