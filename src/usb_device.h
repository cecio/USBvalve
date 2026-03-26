#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <stdbool.h>

// Initialize USB device (MSC + CDC)
void usb_device_init(void);

// Check ramdisk hash consistency. Returns true if valid.
bool usb_device_selftest(void);

#endif // USB_DEVICE_H
