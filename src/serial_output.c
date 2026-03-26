/*
 * USBvalve - CDC Serial Output
 *
 * Wraps TinyUSB CDC for printf-style serial output.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "tusb.h"
#include "serial_output.h"

void serial_printf(const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len > 0) {
        if (tud_cdc_connected()) {
            tud_cdc_write(buf, (uint32_t)len);
            tud_cdc_write_flush();
        }
    }
}

void serial_print(const char* str) {
    if (tud_cdc_connected()) {
        tud_cdc_write(str, (uint32_t)strlen(str));
        tud_cdc_write_flush();
    }
}

void serial_println(const char* str) {
    serial_print(str);
    serial_print("\r\n");
}

void hex_dump(const unsigned char* data, size_t size) {
    char asciitab[17];
    asciitab[16] = '\0';

    for (size_t i = 0; i < size; ++i) {
        serial_printf("%02X", data[i]);

        if (data[i] >= ' ' && data[i] <= '~') {
            asciitab[i % 16] = (char)data[i];
        } else {
            asciitab[i % 16] = '.';
        }

        if ((i + 1) % 8 == 0 || i + 1 == size) {
            serial_print(" ");
            if ((i + 1) % 16 == 0) {
                serial_println(asciitab);
            } else if (i + 1 == size) {
                asciitab[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8) {
                    serial_print(" ");
                }
                for (size_t j = (i + 1) % 16; j < 16; ++j) {
                    serial_print("   ");
                }
                serial_print("|  ");
                serial_println(asciitab);
            }
        }
    }
    serial_print("\r\n");
}
