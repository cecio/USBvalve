/*
 * USBvalve - HID Report Handler
 *
 * Decodes keyboard and mouse HID reports.
 * Called from Core 1 uses tud_cdc_write() directly (no serial_print
 * wrappers, which gate on tud_cdc_connected() and may block).
 */

#include <stdio.h>
#include <string.h>
#include "tusb.h"
#include "hid_handler.h"

// Keycode to ASCII lookup table from TinyUSB
static uint8_t const keycode2ascii[128][2] = { HID_KEYCODE_TO_ASCII };

//--------------------------------------------------------------------
// Internal helpers
//--------------------------------------------------------------------

static inline bool find_key_in_report(hid_keyboard_report_t const* report, uint8_t keycode) {
    for (uint8_t i = 0; i < 6; i++) {
        if (report->keycode[i] == keycode) return true;
    }
    return false;
}

static void check_special_key(uint8_t code, bool *flush) {
    const char *str = NULL;

    switch (code) {
        case HID_KEY_ARROW_RIGHT:       str = "<ARROWRIGHT>"; break;
        case HID_KEY_ARROW_LEFT:        str = "<ARROWLEFT>"; break;
        case HID_KEY_ARROW_DOWN:        str = "<ARROWDOWN>"; break;
        case HID_KEY_ARROW_UP:          str = "<ARROWUP>"; break;
        case HID_KEY_HOME:              str = "<HOME>"; break;
        case HID_KEY_KEYPAD_1:          str = "<KEYPAD_1>"; break;
        case HID_KEY_KEYPAD_2:          str = "<KEYPAD_2>"; break;
        case HID_KEY_KEYPAD_3:          str = "<KEYPAD_3>"; break;
        case HID_KEY_KEYPAD_4:          str = "<KEYPAD_4>"; break;
        case HID_KEY_KEYPAD_5:          str = "<KEYPAD_5>"; break;
        case HID_KEY_KEYPAD_6:          str = "<KEYPAD_6>"; break;
        case HID_KEY_KEYPAD_7:          str = "<KEYPAD_7>"; break;
        case HID_KEY_KEYPAD_8:          str = "<KEYPAD_8>"; break;
        case HID_KEY_KEYPAD_9:          str = "<KEYPAD_9>"; break;
        case HID_KEY_KEYPAD_0:          str = "<KEYPAD_0>"; break;
        case HID_KEY_F1:                str = "<F1>"; break;
        case HID_KEY_F2:                str = "<F2>"; break;
        case HID_KEY_F3:                str = "<F3>"; break;
        case HID_KEY_F4:                str = "<F4>"; break;
        case HID_KEY_F5:                str = "<F5>"; break;
        case HID_KEY_F6:                str = "<F6>"; break;
        case HID_KEY_F7:                str = "<F7>"; break;
        case HID_KEY_F8:                str = "<F8>"; break;
        case HID_KEY_F9:                str = "<F9>"; break;
        case HID_KEY_F10:               str = "<F10>"; break;
        case HID_KEY_F11:               str = "<F11>"; break;
        case HID_KEY_F12:               str = "<F12>"; break;
        case HID_KEY_PRINT_SCREEN:      str = "<PRNT>"; break;
        case HID_KEY_SCROLL_LOCK:       str = "<SCRLL>"; break;
        case HID_KEY_PAUSE:             str = "<PAUSE>"; break;
        case HID_KEY_INSERT:            str = "<INSERT>"; break;
        case HID_KEY_PAGE_UP:           str = "<PAGEUP>"; break;
        case HID_KEY_DELETE:            str = "<DEL>"; break;
        case HID_KEY_END:               str = "<END>"; break;
        case HID_KEY_PAGE_DOWN:         str = "<PAGEDOWN>"; break;
        case HID_KEY_NUM_LOCK:          str = "<NUMLOCK>"; break;
        case HID_KEY_KEYPAD_DIVIDE:     str = "<KEYPAD_DIV>"; break;
        case HID_KEY_KEYPAD_MULTIPLY:   str = "<KEYPAD_MUL>"; break;
        case HID_KEY_KEYPAD_SUBTRACT:   str = "<KEYPAD_SUB>"; break;
        case HID_KEY_KEYPAD_ADD:        str = "<KEYPAD_ADD>"; break;
        case HID_KEY_KEYPAD_DECIMAL:    str = "<KEYPAD_DECIMAL>"; break;
        default: break;
    }

    if (str) {
        tud_cdc_write(str, strlen(str));
        *flush = true;
    }
}

//--------------------------------------------------------------------
// Public API
//--------------------------------------------------------------------

void process_kbd_report(uint8_t const* report, uint16_t len) {
    static uint8_t prev_keycodes[6] = {0};
    static uint8_t prev_modifier = 0;
    bool flush = false;

    // Extract modifier and keycodes depending on report length.
    //  9 bytes: [reportID, modifier, reserved, key0..key5] (report ID prefix)
    //  8 bytes: [modifier, reserved, key0..key5]           (standard boot protocol)
    //  2 bytes: [modifier, key0]                           (short, some Low Speed)
    uint8_t modifier;
    uint8_t keycodes[6] = {0};
    uint8_t num_keys;

    if (len <= 2) {
        // Short report: byte 0 = modifier, byte 1 = single keycode
        modifier = report[0];
        if (len == 2) keycodes[0] = report[1];
        num_keys = 1;
    } else if (len == 9) {
        // 9-byte report: byte 0 = report ID, then standard 8-byte layout
        modifier = report[1];
        // report[2] is reserved
        num_keys = 6;
        for (uint8_t i = 0; i < 6; i++) {
            keycodes[i] = report[3 + i];
        }
    } else {
        // Standard 8-byte boot protocol report
        modifier = report[0];
        // report[1] is reserved
        num_keys = (len - 2 > 6) ? 6 : (uint8_t)(len - 2);
        for (uint8_t i = 0; i < num_keys; i++) {
            keycodes[i] = report[2 + i];
        }
    }

    for (uint8_t i = 0; i < num_keys; i++) {
        uint8_t keycode = keycodes[i];
        if (keycode) {
            // Check if key was already in previous report (held)
            bool found = false;
            for (uint8_t j = 0; j < 6; j++) {
                if (prev_keycodes[j] == keycode) { found = true; break; }
            }
            if (found) continue;

            // New key press
            bool const is_shift = modifier &
                (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
            uint8_t ch = keycode2ascii[keycode][is_shift ? 1 : 0];

            bool const is_ctrl = modifier &
                (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL);
            if (is_ctrl) {
                tud_cdc_write("CTRL+", 5);
                flush = true;
            }

            bool const is_gui = modifier &
                (KEYBOARD_MODIFIER_LEFTGUI | KEYBOARD_MODIFIER_RIGHTGUI);
            if (is_gui) {
                tud_cdc_write("GUI+", 4);
                flush = true;
            }

            bool const is_alt = modifier &
                (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT);
            if (is_alt) {
                tud_cdc_write("ALT+", 4);
                flush = true;
            }

            check_special_key(keycode, &flush);

            if (ch) {
                if (ch == '\r') {
                    tud_cdc_write("\r\n", 2);
                } else {
                    tud_cdc_write(&ch, 1);
                }
                flush = true;
            }
        }
    }

    if (flush) tud_cdc_write_flush();
    prev_modifier = modifier;
    memcpy(prev_keycodes, keycodes, 6);
}

void process_mouse_report(hid_mouse_report_t const* report) {
    static hid_mouse_report_t prev_report = {0};

    char l = report->buttons & MOUSE_BUTTON_LEFT   ? 'L' : '-';
    char m = report->buttons & MOUSE_BUTTON_MIDDLE ? 'M' : '-';
    char r = report->buttons & MOUSE_BUTTON_RIGHT  ? 'R' : '-';

    char tempbuf[32];
    int count = sprintf(tempbuf, "%c%c%c %d %d %d\r\n", l, m, r, report->x, report->y, report->wheel);

    tud_cdc_write(tempbuf, count);
    tud_cdc_write_flush();

    prev_report = *report;
}
