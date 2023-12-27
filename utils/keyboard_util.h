// keyboard_util.h

#ifndef KEYBOARD_UTIL_H
#define KEYBOARD_UTIL_H

#include <libusb-1.0/libusb.h>

#define HIDG_READ_PATH "/dev/hidg0"
#define HIDG_WRITE_PATH "/dev/hidg1"


class KeyboardUtil {
public:
    static int find_keyboard(libusb_device **devs, libusb_device_handle **handle, uint8_t *endpoint_address);
};

#endif // KEYBOARD_UTIL_H

