#include <iostream>
#include <libusb-1.0/libusb.h>
#include <fcntl.h>
#include <unistd.h>
#include "keyboard_util.h"

#define HIDG_PATH "/dev/hidg0"

void send_to_pc(libusb_device_handle *pc_handle, unsigned char *data, int length) {
    int transferred;
    // 0x01은 PC와의 통신을 위한 endpoint 주소라고 가정
    int r = libusb_interrupt_transfer(pc_handle, 0x01, data, length, &transferred, 0);
    if (r != 0) {
        std::cerr << "Error sending data to PC: " << libusb_error_name(r) << std::endl;
    }
}


void cb_transfer(struct libusb_transfer* transfer) {
    if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
        fprintf(stderr, "Transfer not completed: %d\n", transfer->status);
        return;
    }

    unsigned char *data = transfer->buffer;
    for (int i = 0; i < transfer->actual_length; i++) {
        std::cout << "0x" << std::hex << std::uppercase << (int)data[i] << " ";
    }
    std::cout << std::endl;

    // HID report 보내기
    send_to_pc(pc_handle, data, transfer->actual_length);
    // int hidg_fd = open(HIDG_PATH, O_RDWR);
    // if (hidg_fd < 0) {
    //     perror("Failed to open " HIDG_PATH);
    // } else {
    //     write(hidg_fd, transfer->buffer, transfer->actual_length);
    //     close(hidg_fd);
    // }

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

    libusb_close(handle);
    libusb_free_device_list(devs, 1);
}
