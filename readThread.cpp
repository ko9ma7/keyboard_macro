#include <iostream>
#include <libusb-1.0/libusb.h>
#include <fcntl.h>
#include <unistd.h>
#include "keyboard_util.h"
#include "global_vars.h"
#include <chrono>
#include <fstream>
#include <queue>
#include <thread>
#include <sstream>
#include <mutex>
#include <condition_variable>

const int BUFFERED_LOG_COUNT = 100;  // 한 번에 기록할 로그의 최대 개수
int hidg_fd;

void loggerThreadFunc() {
    std::ofstream file("keyboard_logs.txt");

    while (true) {

        std::unique_lock<std::mutex> lock(logMutex);

        logCondition.wait(lock, [] { 
            return isRecording && !logQueue.empty(); 
        });

        if (logQueue.empty()) {
            continue;
        }

        auto logItem = logQueue.front();
        logQueue.pop();
        lock.unlock();

        file << "Time diff: " << logItem.first.count() << "ns, Data: ";
        for (const auto& byte : logItem.second) {
            file << std::hex << std::uppercase << static_cast<int>(byte) << " ";
        }
        file << std::endl;

        // Check for errors
        if (!file) {
            std::cerr << "Error writing to keyboard_logs.txt!" << std::endl;
            // Optionally, you can add more error handling here.
        }
    }

    file.close();
}

void cb_transfer(struct libusb_transfer* transfer) {
    if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
        fprintf(stderr, "Transfer not completed: %d\n", transfer->status);
        return;
    }
    auto currentTimestamp = std::chrono::high_resolution_clock::now();

    if (isStartingToRecord) {
        lastTimestamp = currentTimestamp;
        isStartingToRecord = false;
    }

    if (isRecording) {
        auto elapsedNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTimestamp - lastTimestamp);

        std::vector<unsigned char> dataVector(transfer->buffer, transfer->buffer + transfer->actual_length);

        std::unique_lock<std::mutex> lock(logMutex);
        logQueue.emplace(elapsedNanoseconds, dataVector);
        logCondition.notify_one();
    }
    lastTimestamp = currentTimestamp;

    // HID report 보내기
    if (hidg_fd < 0) {
        perror("Failed to open " HIDG_READ_PATH);
    } else {
        write(hidg_fd, transfer->buffer, transfer->actual_length);
    }

    // 다시 전송을 예약 (계속해서 데이터를 받기 위함)
    libusb_submit_transfer(transfer);
}


void readThreadFunc(libusb_context* ctx) {
    libusb_device **devs;
    libusb_device_handle *handle = NULL;
    ssize_t cnt;
    uint8_t endpoint_address;

    cnt = libusb_get_device_list(ctx, &devs);
    if (cnt < 0) {
        std::cerr << "Failed to get device list." << std::endl;
        return;
    }

    if (find_keyboard(devs, &handle, &endpoint_address) != 0) {
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
    libusb_fill_interrupt_transfer(transfer, handle, endpoint_address, data, sizeof(data), cb_transfer, NULL, 0);
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
