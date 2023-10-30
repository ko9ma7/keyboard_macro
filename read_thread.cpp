#include <iostream>
#include <libusb-1.0/libusb.h>
#include <fcntl.h>
#include <unistd.h>
#include "keyboard_util.h"
#include <chrono>
#include <fstream>
#include <queue>
#include <thread>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include "read_thread.h"

void ReadThread::cb_transfer(struct libusb_transfer* transfer) {
    ReadThread* self = static_cast<ReadThread*>(transfer->user_data);

    if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
        fprintf(stderr, "Transfer not completed: %d\n", transfer->status);
        return;
    }
    auto currentTimestamp = std::chrono::high_resolution_clock::now();

    if (self->isStartingToRecord) {
        self->lastTimestamp = currentTimestamp;
        self->isStartingToRecord = false;
    }

    if (self->isRecording) {
        auto elapsedNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTimestamp - self->lastTimestamp);

        std::vector<unsigned char> dataVector(transfer->buffer, transfer->buffer + transfer->actual_length);

        std::unique_lock<std::mutex> lock(self->logMutex);
        self->logQueue.emplace(elapsedNanoseconds, dataVector);
        self->logCondition.notify_one();
    }
    self->lastTimestamp = currentTimestamp;

    // HID report 보내기
    self->hidg_fd = open("/dev/hidg0", O_RDWR); // HIDG_READ_PATH가 정의되지 않았으므로 임시 경로를 사용
    if (self->hidg_fd < 0) {
        perror("Failed to open /dev/hidg0"); // 경로 수정
    } else {
        write(self->hidg_fd, transfer->buffer, transfer->actual_length);
        close(self->hidg_fd); // 파일 디스크립터를 닫아야 함
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

void ReadThread::setLastTimestamp(const std::chrono::high_resolution_clock::time_point& timestamp) {
    lastTimestamp = timestamp;
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