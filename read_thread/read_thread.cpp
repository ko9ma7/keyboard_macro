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
#include <libusb-1.0/libusb.h>
#include <mutex>
#include <condition_variable>
#include "../utils/keyboard_util.h"
#include "read_thread.h"
#include "../utils/type.h"

void ReadThread::cb_transfer(struct libusb_transfer* transfer) {
    ReadThread* self = static_cast<ReadThread*>(transfer->user_data);

    if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
        fprintf(stderr, "Transfer not completed: %d\n", transfer->status);
        return;
    }

    // HID report 보내기
    if (self->hidg_fd < 0) {
        perror("Failed to open /dev/hidg0"); // 경로 수정
    } else {
        if (self->isStartingToRecord) {
            self->startTime = std::chrono::high_resolution_clock::now();
            self->isStartingToRecord = false;
        }

        auto currentTimestamp = std::chrono::high_resolution_clock::now();
        write(self->hidg_fd, transfer->buffer, transfer->actual_length);

        if (self->isRecording) {
            auto elapsedNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTimestamp - self->startTime);

            std::vector<unsigned char> dataVector(transfer->buffer, transfer->buffer + transfer->actual_length);

            std::unique_lock<std::mutex> lock(self->logMutex);
            self->logQueue.emplace(elapsedNanoseconds, dataVector);
            self->logCondition.notify_one();
        }
    }

    // 다시 전송을 예약 (계속해서 데이터를 받기 위함)
    libusb_submit_transfer(transfer);
}


void ReadThread::readThreadFunc(libusb_context* ctx) {
    libusb_device **devs;
    libusb_device_handle *handle = NULL;
    ssize_t cnt;
    uint8_t endpoint_address;

    cnt = libusb_get_device_list(ctx, &devs);
    if (cnt < 0) {
        std::cerr << "Failed to get device list." << std::endl;
        return;
    }

    if (KeyboardUtil::find_keyboard(devs, &handle, &endpoint_address) != 0) {
        std::cerr << "No keyboard found." << std::endl;
        libusb_free_device_list(devs, 1);
        return;
    }

    unsigned char data[8];
    libusb_transfer* transfer;
    transfer = libusb_alloc_transfer(0);
    if (!transfer) {
        std::cerr << "Failed to allocate transfer." << std::endl;
        return;
    }

    hidg_fd = open(HIDG_READ_PATH, O_RDWR);
    libusb_fill_interrupt_transfer(transfer, handle, endpoint_address, data, sizeof(data), &ReadThread::cb_transfer, this, 0);
    int r = libusb_submit_transfer(transfer);
    if (r != 0) {
        std::cerr << "Error submitting transfer: " << libusb_error_name(r) << std::endl;
        return;
    }

    while (true) {
        r = libusb_handle_events(ctx);
        if (r != 0) {
            std::cerr << "Error handling events: " << libusb_error_name(r) << std::endl;
            break;
        }
    }

    close(hidg_fd);
    libusb_close(handle);
    libusb_free_device_list(devs, 1);
}

std::queue<std::pair<std::chrono::nanoseconds, std::vector<unsigned char>>>& ReadThread::getLogQueue() {
    return logQueue;
}

std::condition_variable& ReadThread::getLogCondition() {
    return logCondition;
}

std::mutex& ReadThread::getLogMutex() {
    return logMutex;
}

void ReadThread::replayMacro(const std::string& logFilename, std::function<void(const std::string&)> eventCallback) {
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

    struct timespec startReplayTime;
    clock_gettime(CLOCK_MONOTONIC, &startReplayTime);

    for (const auto& event : events) {
        // 종료 요청이 들어왔는지 확인
        std::cout<<stopRequested<<'\n';
        if (stopRequested) {
            std::cout << "매크로 재생 중단\n";
            unsigned char stopReport[] = {0, 0, 0, 0, 0, 0, 0, 0};
            write(hidg_fd, stopReport, sizeof(stopReport));
            break; // 종료 플래그가 설정되면 루프를 종료
        }

        struct timespec targetTime;
        targetTime.tv_sec = startReplayTime.tv_sec + (event.delay / 1000000000);
        targetTime.tv_nsec = startReplayTime.tv_nsec + (event.delay % 1000000000);

        // timespec 구조체 정규화
        if (targetTime.tv_nsec >= 1000000000) {
            targetTime.tv_sec += 1;
            targetTime.tv_nsec -= 1000000000;
        }

        struct timespec remaining;
        while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &targetTime, &remaining) == EINTR) {
            targetTime = remaining;
        }   
        
        ssize_t bytes_written = write(hidg_fd, &event.data[0], event.data.size());

        // Write error check
        if (bytes_written == -1) {
            std::cerr << "Failed to write to " HIDG_READ_PATH ". Error: " << strerror(errno) << std::endl;
            continue;
        } else if (bytes_written != static_cast<ssize_t>(event.data.size())) {
            std::cerr << "Partial write to " HIDG_READ_PATH ". Only " << bytes_written << " out of " << event.data.size() << " bytes were written." << std::endl;
        }

        if (eventCallback) {  // 콜백 함수가 제공되었는지 확인
            std::string eventDescription = "Event at " + std::to_string(event.delay) + "ns";
            eventCallback(eventDescription);  // 콜백 호출
        }
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

std::vector<KeyMacro::KeyEvent> ReadThread::readMacroFile(const std::string& filename) {
    std::string fullPath = "save/" + filename;  // 전체 파일 경로 생성
    std::ifstream logFile(fullPath);  // 전체 경로를 사용하여 파일 열기
    std::string line;
    std::vector<KeyMacro::KeyEvent> events;

    if (!logFile.is_open()) {
        std::cerr << "Error opening log file." << std::endl;
        return events;
    }

    // Load all events into memory
    
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
    return events;
}
