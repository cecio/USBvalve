#ifndef SERIAL_OUTPUT_H
#define SERIAL_OUTPUT_H

#include <stddef.h>

// Printf to CDC serial
void serial_printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

// Print string to CDC serial
void serial_print(const char* str);

// Print string + newline to CDC serial
void serial_println(const char* str);

// Hex dump to CDC serial
void hex_dump(const unsigned char* data, size_t size);

#endif // SERIAL_OUTPUT_H
