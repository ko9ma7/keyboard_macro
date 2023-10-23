#include <iostream>
#include <libusb-1.0/libusb.h>
#include <fcntl.h>
#include <unistd.h>
#include "keyboard_util.h"

void cb_write_transfer(struct libusb_transfer* transfer) {
    if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
        fprintf(stderr, "Write transfer not completed: %d\n", transfer->status);
        return;
    }

    // Handle the received data as needed
    // ...

    // Optionally, you can re-submit the transfer if you want to receive more data
    libusb_submit_transfer(transfer);
}

void writeThreadFunc(libusb_context* ctx) {
    libusb_device **devs;
    libusb_device_handle *handle = NULL;
    uint8_t endpoint_address;

    ssize_t cnt = libusb_get_device_list(ctx, &devs);
    if (cnt < 0) {
        std::cerr << "Failed to get device list." << std::endl;
        return;
    }

    if (find_keyboard(devs, &handle, &endpoint_address) != 0) {
        // Handle the error
        return;
    }

    unsigned char data[8];
    libusb_transfer* transfer = libusb_alloc_transfer(0);
    if (!transfer) {
        // Handle the error
        return;
    }

    libusb_fill_interrupt_transfer(transfer, handle, endpoint_address, data, sizeof(data), cb_write_transfer, NULL, 5000);
    int r = libusb_submit_transfer(transfer);
    if (r != 0) {
        // Handle the error
        return;
    }

    while (true) {
        r = libusb_handle_events(ctx);
        if (r != 0) {
            // Handle the error
            break;
        }
    }

    // Cleanup
    libusb_close(handle);
    libusb_free_device_list(devs, 1);
}