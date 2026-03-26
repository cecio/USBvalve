#ifndef USB_HOST_H
#define USB_HOST_H

// Initialize USB host on Core 1 (PIO USB)
void usb_host_init(void);

// Core 1 entry point
void core1_entry(void);

#endif // USB_HOST_H
