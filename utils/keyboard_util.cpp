#include "keyboard_util.h"
#include <iostream>

int KeyboardUtil::find_keyboard(libusb_device **devs, libusb_device_handle **handle, uint8_t *endpoint_address) {
    libusb_device *dev;
    int i = 0, r;

    while ((dev = devs[i++]) != NULL) {
        libusb_device_descriptor desc;
        r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            std::cerr << "Failed to get device descriptor." << std::endl;
            return -1;
        }

        if (desc.bDeviceClass == LIBUSB_CLASS_PER_INTERFACE) {
            libusb_config_descriptor *config;
            libusb_get_config_descriptor(dev, 0, &config);

            for (uint8_t j = 0; j < config->bNumInterfaces; j++) {
                const libusb_interface *inter = &config->interface[j];
                for (int k = 0; k < inter->num_altsetting; k++) {
                    const libusb_interface_descriptor *interdesc = &inter->altsetting[k];
                    if (interdesc->bInterfaceClass == LIBUSB_CLASS_HID && interdesc->bInterfaceProtocol == 1) {
                        for (int l = 0; l < interdesc->bNumEndpoints; l++) {
                            if (interdesc->endpoint[l].bEndpointAddress & LIBUSB_ENDPOINT_IN) {
                                *endpoint_address = interdesc->endpoint[l].bEndpointAddress;
                            }
                        }
                        *handle = libusb_open_device_with_vid_pid(NULL, desc.idVendor, desc.idProduct);
                        if (libusb_kernel_driver_active(*handle, 0) == 1) {
                            if (libusb_detach_kernel_driver(*handle, 0) != 0) {
                                std::cerr << "Failed to detach kernel driver." << std::endl;
                            }
                        }
                        if (*handle) {
                            libusb_free_config_descriptor(config);
                            return 0; // Success
                        }
                    }
                }
            }

            libusb_free_config_descriptor(config);
        }
    }

    return -1; // Failed to find a keyboard
}