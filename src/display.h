#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

// Initialize display (SSD1306 I2C or GC9A01 SPI depending on build config)
void display_init(void);

// Clear entire framebuffer
void display_clear(void);

// Write framebuffer to display
void display_update(void);

// Set text cursor position
void display_set_cursor(uint8_t x, uint8_t y);

// Get current cursor Y position
uint8_t display_get_cursor_y(void);

// Get display dimensions
uint8_t display_width(void);
uint8_t display_height(void);

// Print string at current cursor (wraps, advances cursor)
void display_print(const char* str);

// Get pixel value at (x, y)
uint8_t display_get_pixel(uint8_t x, uint8_t y);

// Set pixel at (x, y) to color (0=black, 1=white)
void display_draw_pixel(uint8_t x, uint8_t y, uint8_t color);

// Fill rectangle with color
void display_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);

// High-level functions

// Scroll display up by N pixels
void display_scroll_up(uint8_t pixels);

// Check if scroll is needed and perform it
void display_check_and_scroll(void);

// Print message to display + serial
void printout(const char* str);

// Clear display and show version header
void cls(void);

#endif // DISPLAY_H
