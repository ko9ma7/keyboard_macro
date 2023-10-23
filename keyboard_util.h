// keyboard_util.h

#pragma once

#include <libusb-1.0/libusb.h>

int find_keyboard(libusb_device **devs, libusb_device_handle **handle, uint8_t *endpoint_address);
