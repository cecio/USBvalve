#ifndef USB_CONFIG_H
#define USB_CONFIG_H

// Project version
#define VERSION "USBvalve - 1.0.0"

// USB anti-detection settings
//
// Set USB IDs strings and numbers, to avoid possible detections.
// Remember that you can cusotmize FAKE_DISK_BLOCK_NUM as well
// for the same reason. Also DISK_LABEL in ramdisk.h can be changed.
//
// You can see here for inspiration: https://the-sz.com/products/usbid/
//
// Example:
//             0x0951 0x16D5    VENDORID_STR: Kingston   PRODUCTID_STR: DataTraveler
//
#define USB_VENDORID       0x0951
#define USB_PRODUCTID      0x16D5
#define USB_DESCRIPTOR     "DataTraveler"
#define USB_MANUF          "Kingston"
#define USB_SERIAL         "123456789A"
#define USB_VENDORID_STR   "Kingston"        // Up to 8 chars
#define USB_PRODUCTID_STR  "DataTraveler"    // Up to 16 chars
#define USB_VERSION_STR    "1.0"             // Up to 4 chars

// Disk configuration
#define DISK_BLOCK_NUM       0x150
#define FAKE_DISK_BLOCK_NUM  0x800
#define DISK_BLOCK_SIZE      0x200

// Block locations
#define BLOCK_AUTORUN    102
#define BLOCK_README     100

// Debug
#define MAX_DUMP_BYTES   16

// Hash validation
#define BYTES_TO_HASH         (512 * 2)
#define BYTES_TO_HASH_OFFSET  7
#define VALID_HASH            2362816530U

// GPIO pins
#define HOST_PIN_DP   14
#define LED_PIN       25
#define BUTTON_PIN    0    // External reset button (active-low, pulled up)

// Display
#define I2C_SDA_PIN   4
#define I2C_SCL_PIN   5
#define I2C_ADDRESS   0x3C
#define OLED_WIDTH    128
// OLED_HEIGHT is set via CMake: -DOLED_HEIGHT=32 (default) or -DOLED_HEIGHT=64
#ifndef OLED_HEIGHT
#define OLED_HEIGHT   32
#endif

// HID
#define LANGUAGE_ID   0x0409

#endif // USB_CONFIG_H
