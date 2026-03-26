/*
 * USBvalve - Display Driver
 *
 * Two implementations selected at compile time:
 *   - Default: SSD1306 OLED via I2C (128x32 or 128x64)
 *   - PIWATCH: GC9A01 round TFT via SPI (240x240)
 */

#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "display.h"
#include "serial_output.h"
#include "usb_config.h"
#include "font6x8.h"

#define FONT_W 6
#define FONT_H 8

#ifdef PIWATCH

//====================================================================
// PIWATCH: GC9A01 round TFT (240x240) via SPI
//====================================================================

#include "hardware/spi.h"
#include "background.h"

// GC9A01 pin definitions
#define GFX_DC    8
#define GFX_CS    9
#define GFX_CLK   10
#define GFX_MOSI  11
#define GFX_RST   12
#define GFX_BL    25   // Backlight (shared with LED_PIN on non-PIWATCH)

#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// Text area for event messages
#define TEXT_X      70
#define TEXT_Y_START 80
#define TEXT_Y_MAX  120
#define TEXT_AREA_X  60
#define TEXT_AREA_Y  70
#define TEXT_AREA_W 140
#define TEXT_AREA_H  75

// Cursor state
static uint16_t cursor_x = TEXT_X;
static uint16_t cursor_y = TEXT_Y_START;

// Text color (magenta in RGB565)
#define COLOR_MAGENTA 0xF81F
#define COLOR_BLACK   0x0000

//--------------------------------------------------------------------
// Low-level SPI helpers
//--------------------------------------------------------------------
static void gc9a01_cmd(uint8_t cmd) {
    gpio_put(GFX_DC, 0); // Command mode
    gpio_put(GFX_CS, 0);
    spi_write_blocking(spi1, &cmd, 1);
    gpio_put(GFX_CS, 1);
}

static void gc9a01_data(const uint8_t *data, size_t len) {
    gpio_put(GFX_DC, 1); // Data mode
    gpio_put(GFX_CS, 0);
    spi_write_blocking(spi1, data, len);
    gpio_put(GFX_CS, 1);
}

static void gc9a01_data8(uint8_t val) {
    gc9a01_data(&val, 1);
}

static void gc9a01_data16(uint16_t val) {
    uint8_t buf[2] = {val >> 8, val & 0xFF};
    gc9a01_data(buf, 2);
}

static void gc9a01_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    gc9a01_cmd(0x2A); // Column address set
    gc9a01_data16(x0);
    gc9a01_data16(x1);
    gc9a01_cmd(0x2B); // Row address set
    gc9a01_data16(y0);
    gc9a01_data16(y1);
    gc9a01_cmd(0x2C); // Memory write
}

//--------------------------------------------------------------------
// GC9A01 Init
//--------------------------------------------------------------------
void display_init(void) {
    // Init SPI at 40MHz
    spi_init(spi1, 40000000);
    gpio_set_function(GFX_CLK, GPIO_FUNC_SPI);
    gpio_set_function(GFX_MOSI, GPIO_FUNC_SPI);

    // Init control pins
    gpio_init(GFX_CS);
    gpio_set_dir(GFX_CS, GPIO_OUT);
    gpio_put(GFX_CS, 1);

    gpio_init(GFX_DC);
    gpio_set_dir(GFX_DC, GPIO_OUT);

    gpio_init(GFX_RST);
    gpio_set_dir(GFX_RST, GPIO_OUT);

    gpio_init(GFX_BL);
    gpio_set_dir(GFX_BL, GPIO_OUT);

    // Hardware reset
    gpio_put(GFX_RST, 1);
    sleep_ms(10);
    gpio_put(GFX_RST, 0);
    sleep_ms(10);
    gpio_put(GFX_RST, 1);
    sleep_ms(120);

    // GC9A01 initialization sequence
    gc9a01_cmd(0xEF);
    gc9a01_cmd(0xEB); gc9a01_data8(0x14);
    gc9a01_cmd(0xFE); // Inter register enable 1
    gc9a01_cmd(0xEF); // Inter register enable 2

    gc9a01_cmd(0xEB); gc9a01_data8(0x14);
    gc9a01_cmd(0x84); gc9a01_data8(0x40);
    gc9a01_cmd(0x85); gc9a01_data8(0xFF);
    gc9a01_cmd(0x86); gc9a01_data8(0xFF);
    gc9a01_cmd(0x87); gc9a01_data8(0xFF);
    gc9a01_cmd(0x88); gc9a01_data8(0x0A);
    gc9a01_cmd(0x89); gc9a01_data8(0x21);
    gc9a01_cmd(0x8A); gc9a01_data8(0x00);
    gc9a01_cmd(0x8B); gc9a01_data8(0x80);
    gc9a01_cmd(0x8C); gc9a01_data8(0x01);
    gc9a01_cmd(0x8D); gc9a01_data8(0x01);
    gc9a01_cmd(0x8E); gc9a01_data8(0xFF);
    gc9a01_cmd(0x8F); gc9a01_data8(0xFF);

    gc9a01_cmd(0xB6); // Display function control
    uint8_t b6[] = {0x00, 0x00};
    gc9a01_data(b6, 2);

    gc9a01_cmd(0x36); // Memory access control: MV+BGR (rotation=1)
    gc9a01_data8(0x28);

    gc9a01_cmd(0x3A); // Pixel format: 16-bit RGB565
    gc9a01_data8(0x05);

    gc9a01_cmd(0x90);
    uint8_t d90[] = {0x08, 0x08, 0x08, 0x08};
    gc9a01_data(d90, 4);

    gc9a01_cmd(0xBD); gc9a01_data8(0x06);
    gc9a01_cmd(0xBC); gc9a01_data8(0x00);
    gc9a01_cmd(0xFF);
    uint8_t dff[] = {0x60, 0x01, 0x04};
    gc9a01_data(dff, 3);

    gc9a01_cmd(0xC3); gc9a01_data8(0x13); // Voltage reg 1a
    gc9a01_cmd(0xC4); gc9a01_data8(0x13); // Voltage reg 1b
    gc9a01_cmd(0xC9); gc9a01_data8(0x22);

    gc9a01_cmd(0xBE); gc9a01_data8(0x11);
    gc9a01_cmd(0xE1); gc9a01_data8(0x10);
    gc9a01_cmd(0xDF);
    uint8_t ddf[] = {0x21, 0x0C, 0x02};
    gc9a01_data(ddf, 3);

    // Gamma
    gc9a01_cmd(0xF0);
    uint8_t gp[] = {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A};
    gc9a01_data(gp, 6);

    gc9a01_cmd(0xF1);
    uint8_t gn[] = {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F};
    gc9a01_data(gn, 6);

    gc9a01_cmd(0xF2);
    uint8_t g2p[] = {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A};
    gc9a01_data(g2p, 6);

    gc9a01_cmd(0xF3);
    uint8_t g2n[] = {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F};
    gc9a01_data(g2n, 6);

    gc9a01_cmd(0xED);
    uint8_t ded[] = {0x1B, 0x0B};
    gc9a01_data(ded, 2);

    gc9a01_cmd(0xAE); gc9a01_data8(0x77);
    gc9a01_cmd(0xCD); gc9a01_data8(0x63);

    gc9a01_cmd(0x70);
    uint8_t d70[] = {0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03};
    gc9a01_data(d70, 9);

    gc9a01_cmd(0xE8); gc9a01_data8(0x34);

    gc9a01_cmd(0x62);
    uint8_t d62[] = {0x18, 0x0D, 0x71, 0xED, 0x70, 0x70,
                     0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70};
    gc9a01_data(d62, 12);

    gc9a01_cmd(0x63);
    uint8_t d63[] = {0x18, 0x11, 0x71, 0xF1, 0x70, 0x70,
                     0x18, 0x13, 0x71, 0xF3, 0x70, 0x70};
    gc9a01_data(d63, 12);

    gc9a01_cmd(0x64);
    uint8_t d64[] = {0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07};
    gc9a01_data(d64, 7);

    gc9a01_cmd(0x66);
    uint8_t d66[] = {0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00};
    gc9a01_data(d66, 10);

    gc9a01_cmd(0x67);
    uint8_t d67[] = {0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98};
    gc9a01_data(d67, 10);

    gc9a01_cmd(0x74);
    uint8_t d74[] = {0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00};
    gc9a01_data(d74, 7);

    gc9a01_cmd(0x98);
    uint8_t d98[] = {0x3E, 0x07};
    gc9a01_data(d98, 2);

    gc9a01_cmd(0x35); // Tearing effect line on
    gc9a01_cmd(0x21); // Display inversion on (IPS)
    gc9a01_cmd(0x11); // Sleep out
    sleep_ms(120);
    gc9a01_cmd(0x29); // Display on
    sleep_ms(20);

    // Fill screen black
    gc9a01_set_window(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
    gpio_put(GFX_DC, 1);
    gpio_put(GFX_CS, 0);
    for (uint32_t i = 0; i < TFT_WIDTH * TFT_HEIGHT; i++) {
        uint8_t buf[2] = {0, 0};
        spi_write_blocking(spi1, buf, 2);
    }
    gpio_put(GFX_CS, 1);

    // Draw background bitmap (210x210 at offset 10,0)
    gc9a01_set_window(10, 0, 10 + 210 - 1, 210 - 1);
    gpio_put(GFX_DC, 1);
    gpio_put(GFX_CS, 0);
    for (uint32_t i = 0; i < 44100; i++) {
        uint16_t pixel = background[i];
        uint8_t buf[2] = {pixel >> 8, pixel & 0xFF};
        spi_write_blocking(spi1, buf, 2);
    }
    gpio_put(GFX_CS, 1);

    sleep_ms(2000);

    // Backlight on
    gpio_put(GFX_BL, 1);
}

//--------------------------------------------------------------------
// TFT drawing helpers
//--------------------------------------------------------------------
static void tft_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
    if (x + w > TFT_WIDTH) w = TFT_WIDTH - x;
    if (y + h > TFT_HEIGHT) h = TFT_HEIGHT - y;

    gc9a01_set_window(x, y, x + w - 1, y + h - 1);
    gpio_put(GFX_DC, 1);
    gpio_put(GFX_CS, 0);
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
        uint8_t buf[2] = {hi, lo};
        spi_write_blocking(spi1, buf, 2);
    }
    gpio_put(GFX_CS, 1);
}

static void tft_draw_char(uint16_t x, uint16_t y, char c, uint16_t color) {
    if (c < 0x20 || c > 0x7E) return;
    uint8_t idx = c - 0x20;

    // Draw character pixel by pixel using individual 1x1 rects
    for (uint8_t col = 0; col < FONT_W; col++) {
        uint8_t line = font6x8[idx][col];
        for (uint8_t row = 0; row < FONT_H; row++) {
            if (line & (1 << row)) {
                gc9a01_set_window(x + col, y + row, x + col, y + row);
                gc9a01_data16(color);
            }
        }
    }
}

//--------------------------------------------------------------------
// Display interface implementation
//--------------------------------------------------------------------
void display_clear(void) {
    cursor_x = TEXT_X;
    cursor_y = TEXT_Y_START;
}

void display_update(void) {
    // TFT updates are immediate, no framebuffer to flush
}

void display_set_cursor(uint8_t x, uint8_t y) {
    cursor_x = x;
    cursor_y = y;
}

uint8_t display_get_cursor_y(void) {
    return (uint8_t)cursor_y;
}

uint8_t display_width(void) {
    return TFT_WIDTH;
}

uint8_t display_height(void) {
    return TFT_HEIGHT;
}

void display_print(const char* str) {
    while (*str) {
        if (*str == '\n') {
            cursor_x = TEXT_X;
            cursor_y += FONT_H + 2;
        } else if (*str == '\r') {
            cursor_x = TEXT_X;
        } else {
            tft_draw_char(cursor_x, cursor_y, *str, COLOR_MAGENTA);
            cursor_x += FONT_W;
        }
        str++;
    }
}

uint8_t display_get_pixel(uint8_t x, uint8_t y) {
    (void)x; (void)y;
    return 0; // Not used for TFT
}

void display_draw_pixel(uint8_t x, uint8_t y, uint8_t color) {
    gc9a01_set_window(x, y, x, y);
    gc9a01_data16(color ? COLOR_MAGENTA : COLOR_BLACK);
}

void display_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color) {
    tft_fill_rect(x, y, w, h, color ? COLOR_MAGENTA : COLOR_BLACK);
}

void display_scroll_up(uint8_t pixels) {
    (void)pixels;
    // TFT: no pixel scroll, just clear text area on overflow
}

void display_check_and_scroll(void) {
    if (cursor_y > TEXT_Y_MAX) {
        cls();
    }
}

void printout(const char* str) {
    display_check_and_scroll();

    // Skip leading newline for TFT
    if (str[0] == '\n') {
        cursor_x = TEXT_X;
        cursor_y += FONT_H + 2;
        display_print(str + 1);
    } else {
        display_print(str);
    }

    // Also output on CDC serial
    serial_println(str);
}

void cls(void) {
    // Clear the text area with a black rounded rect
    tft_fill_rect(TEXT_AREA_X, TEXT_AREA_Y, TEXT_AREA_W, TEXT_AREA_H, COLOR_BLACK);
    cursor_x = TEXT_X;
    cursor_y = TEXT_Y_START;
    display_print(VERSION);
    cursor_x = TEXT_X;
    cursor_y = TEXT_Y_START + FONT_H + 2;
    display_print("-----------------");
}

#else // !PIWATCH

//====================================================================
// Default: SSD1306 OLED via I2C (128x32 or 128x64)
//====================================================================

#include "hardware/i2c.h"

// Framebuffer
#define FB_SIZE (OLED_WIDTH * OLED_HEIGHT / 8)
static uint8_t framebuffer[FB_SIZE];

// Cursor state
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;

//--------------------------------------------------------------------
// Low-level I2C helpers
//--------------------------------------------------------------------
static void ssd1306_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd}; // Co=0, D/C#=0 (command)
    i2c_write_blocking(i2c0, I2C_ADDRESS, buf, 2, false);
}

//--------------------------------------------------------------------
// Init
//--------------------------------------------------------------------
void display_init(void) {
    // Init I2C at 400kHz
    i2c_init(i2c0, 400000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // Wait for display to power up
    sleep_ms(100);

    // SSD1306 initialization sequence (works for 128x32 and 128x64)
    ssd1306_cmd(0xAE); // Display off

    ssd1306_cmd(0xD5); // Set display clock divide ratio
    ssd1306_cmd(0x80); // Suggested ratio

    ssd1306_cmd(0xA8); // Set multiplex ratio
    ssd1306_cmd(OLED_HEIGHT - 1);

    ssd1306_cmd(0xD3); // Set display offset
    ssd1306_cmd(0x00); // No offset

    ssd1306_cmd(0x40); // Set start line to 0

    ssd1306_cmd(0x8D); // Charge pump
    ssd1306_cmd(0x14); // Enable charge pump

    ssd1306_cmd(0x20); // Memory addressing mode
    ssd1306_cmd(0x00); // Horizontal addressing mode

    ssd1306_cmd(0xA1); // Segment re-map (column 127 mapped to SEG0)
    ssd1306_cmd(0xC8); // COM output scan direction (remapped)

    ssd1306_cmd(0xDA); // Set COM pins hardware configuration
#if OLED_HEIGHT == 64
    ssd1306_cmd(0x12); // Alternate COM pin config for 128x64
#else
    ssd1306_cmd(0x02); // Sequential COM pin config for 128x32
#endif

    ssd1306_cmd(0x81); // Set contrast
    ssd1306_cmd(0x8F); // Medium contrast

    ssd1306_cmd(0xD9); // Set pre-charge period
    ssd1306_cmd(0xF1);

    ssd1306_cmd(0xDB); // Set VCOMH deselect level
    ssd1306_cmd(0x40);

    ssd1306_cmd(0xA4); // Entire display ON (follow RAM)
    ssd1306_cmd(0xA6); // Normal display (not inverted)

    ssd1306_cmd(0xAF); // Display on

    // Clear framebuffer and update
    memset(framebuffer, 0, FB_SIZE);
    display_update();
}

//--------------------------------------------------------------------
// Framebuffer operations
//--------------------------------------------------------------------
void display_clear(void) {
    memset(framebuffer, 0, FB_SIZE);
    cursor_x = 0;
    cursor_y = 0;
}

void display_update(void) {
    // Set column address range
    ssd1306_cmd(0x21); // Column address
    ssd1306_cmd(0);    // Start
    ssd1306_cmd(OLED_WIDTH - 1); // End

    // Set page address range
    ssd1306_cmd(0x22); // Page address
    ssd1306_cmd(0);    // Start
    ssd1306_cmd((OLED_HEIGHT / 8) - 1); // End

    // Send framebuffer data
    for (uint16_t i = 0; i < FB_SIZE; i += 128) {
        uint8_t buf[129];
        buf[0] = 0x40; // Co=0, D/C#=1 (data)
        uint16_t chunk = (FB_SIZE - i < 128) ? (FB_SIZE - i) : 128;
        memcpy(&buf[1], &framebuffer[i], chunk);
        i2c_write_blocking(i2c0, I2C_ADDRESS, buf, chunk + 1, false);
    }
}

void display_set_cursor(uint8_t x, uint8_t y) {
    cursor_x = x;
    cursor_y = y;
}

uint8_t display_get_cursor_y(void) {
    return cursor_y;
}

uint8_t display_width(void) {
    return OLED_WIDTH;
}

uint8_t display_height(void) {
    return OLED_HEIGHT;
}

uint8_t display_get_pixel(uint8_t x, uint8_t y) {
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return 0;
    uint16_t byte_idx = x + (y / 8) * OLED_WIDTH;
    uint8_t bit_idx = y & 7;
    return (framebuffer[byte_idx] >> bit_idx) & 1;
}

void display_draw_pixel(uint8_t x, uint8_t y, uint8_t color) {
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    uint16_t byte_idx = x + (y / 8) * OLED_WIDTH;
    uint8_t bit_idx = y & 7;
    if (color) {
        framebuffer[byte_idx] |= (1 << bit_idx);
    } else {
        framebuffer[byte_idx] &= ~(1 << bit_idx);
    }
}

void display_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color) {
    for (uint8_t j = y; j < y + h && j < OLED_HEIGHT; j++) {
        for (uint8_t i = x; i < x + w && i < OLED_WIDTH; i++) {
            display_draw_pixel(i, j, color);
        }
    }
}

//--------------------------------------------------------------------
// Text rendering
//--------------------------------------------------------------------
static void draw_char(uint8_t x, uint8_t y, char c) {
    if (c < 0x20 || c > 0x7E) return;
    uint8_t idx = c - 0x20;
    for (uint8_t col = 0; col < FONT_W; col++) {
        uint8_t line = font6x8[idx][col];
        for (uint8_t row = 0; row < FONT_H; row++) {
            if (line & (1 << row)) {
                display_draw_pixel(x + col, y + row, 1);
            }
        }
    }
}

void display_print(const char* str) {
    while (*str) {
        if (*str == '\n') {
            cursor_x = 0;
            cursor_y += FONT_H;
        } else if (*str == '\r') {
            cursor_x = 0;
        } else {
            if (cursor_x + FONT_W > OLED_WIDTH) {
                cursor_x = 0;
                cursor_y += FONT_H;
            }
            draw_char(cursor_x, cursor_y, *str);
            cursor_x += FONT_W;
        }
        str++;
    }
}

//--------------------------------------------------------------------
// High-level display functions
//--------------------------------------------------------------------

void display_scroll_up(uint8_t pixels) {
    for (uint8_t y = 0; y < OLED_HEIGHT - pixels; y++) {
        for (uint8_t x = 0; x < OLED_WIDTH; x++) {
            uint8_t color = display_get_pixel(x, y + pixels);
            display_draw_pixel(x, y, color);
        }
    }
    display_fill_rect(0, OLED_HEIGHT - pixels, OLED_WIDTH, pixels, 0);
    display_update();
}

void display_check_and_scroll(void) {
    if ((cursor_y + 16) > OLED_HEIGHT) {
        display_scroll_up(8);
        cursor_y -= 8;
    }
}

void printout(const char* str) {
    display_check_and_scroll();
    display_print(str);
    display_update();

    // Also output on CDC serial
    serial_println(str);
}

void cls(void) {
    display_clear();
    display_set_cursor(0, 0);
    printout(VERSION);
    printout("\n-----------------");
}

#endif // PIWATCH
