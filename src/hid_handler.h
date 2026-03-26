#ifndef HID_HANDLER_H
#define HID_HANDLER_H

#include "tusb.h"

// Process a keyboard HID report (len needed for short-report devices)
void process_kbd_report(uint8_t const* report, uint16_t len);

// Process a mouse HID report
void process_mouse_report(hid_mouse_report_t const* report);

#endif // HID_HANDLER_H
