/*
  USBvalve
  
  written by Cesare Pizzi
  This project extensively reuse code done by Adafruit and TinyUSB. Please support them!
*/

/*********************************************************************
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  MIT license, check LICENSE for more information
  Copyright (c) 2019 Ha Thach for Adafruit Industries
  All text above, and the splash screen below must be included in
  any redistribution
*********************************************************************/

#include <pio_usb.h>
#include "Adafruit_TinyUSB.h"
#include "SSD1306AsciiWire.h"

//
// BADUSB detector section
//

/*
 * Requirements:
 * - [Pico-PIO-USB](https://github.com/sekigon-gonnoc/Pico-PIO-USB) library
 * - 2 consecutive GPIOs: D+ is defined by HOST_PIN_DP (gpio2), D- = D+ +1 (gpio3)
 * - CPU Speed must be either 120 or 240 Mhz. Selected via "Menu -> CPU Speed"
 */

#define HOST_PIN_DP 14      // Pin used as D+ for host, D- = D+ + 1
#define LANGUAGE_ID 0x0409  // English

// USB Host object
Adafruit_USBH_Host USBHost;

// END of BADUSB detector section

// Define vars for OLED screen
#define I2C_ADDRESS 0x3C  // 0X3C+SA0 - 0x3C or 0x3D
#define RST_PIN -1        // Define proper RST_PIN if required.
#define OLED_HEIGHT 32    // 64 or 32 depending on the OLED
#define OLED_LINES (OLED_HEIGHT / 8)
SSD1306AsciiWire oled;

// Define the dimension of RAM DISK. We have a "real" one (for which
// a real array is created) and a "fake" one, presented to the OS
#define DISK_BLOCK_NUM 0x150
#define FAKE_DISK_BLOCK_NUM 0x800
#define DISK_BLOCK_SIZE 0x200
#include "ramdisk.h"
#include "quark.h"

Adafruit_USBD_MSC usb_msc;

// Eject button to demonstrate medium is not ready e.g SDCard is not present
// whenever this button is pressed and hold, it will report to host as not ready
#if defined(ARDUINO_SAMD_CIRCUITPLAYGROUND_EXPRESS) || defined(ARDUINO_NRF52840_CIRCUITPLAY)
#define BTN_EJECT 4  // Left Button
bool activeState = true;

#elif defined(ARDUINO_FUNHOUSE_ESP32S2)
#define BTN_EJECT BUTTON_DOWN
bool activeState = true;

#elif defined PIN_BUTTON1
#define BTN_EJECT PIN_BUTTON1
bool activeState = false;
#endif

//
// USBvalve globals
//
#define VERSION "USBvalve - 0.9.1"
boolean readme = false;
boolean autorun = false;
boolean written = false;
boolean written_reported = false;

// Set USB IDs strings, you can customize may be to avoid detection?
// Remember that you can cusotmize FAKE_DISK_BLOCK_NUM as well
// for the same reason.
// You can see here for inspiration: https://the-sz.com/products/usbid/
//
// Example:
//             0x0951 0x16D5    VENDORID: Kingston   PRODUCTID: DataTraveler
//
#define USB_VENDORID "Watchdog"   // Up to 8 chars
#define USB_PRODUCTID "USBvalve"  // Up to 16 chars
#define USB_VERSION "1.0"         // Up to 4 chars

#define BLOCK_AUTORUN 102       // Block where Autorun.inf file is saved
#define BLOCK_README 100        // Block where README.txt file is saved
#define MAX_DUMP_BYTES 16       // Used by the dump of the debug facility: do not increase this too much
#define BYTES_TO_HASH 512 * 2   // Number of bytes of the RAM disk used to check consistency
#define BYTES_TO_HASH_OFFSET 7  // Starting sector to check for consistency (FAT_DIRECTORY is 7)

// Burned hash to check consistency
u8 valid_hash[WIDTH] = {
  0x35, 0x98, 0x95, 0x97, 0xC7, 0x70, 0xD3, 0xE4,
  0xDD, 0x84, 0x71, 0x1D, 0x55, 0xD2, 0xE5, 0xA4,
  0x6C, 0x28, 0x84, 0xF6, 0xE1, 0x02, 0xD1, 0x74,
  0x2F, 0xE9, 0x92, 0xAD, 0xAD, 0x74, 0x71, 0xF0,
  0x37, 0xFF, 0x79, 0x39, 0xDC, 0x20, 0x56, 0x26,
  0xFE, 0xC7, 0x9A, 0x4E, 0x3A, 0x27, 0x65, 0x81
};

u8 computed_hash[WIDTH] = { 0x00 };

// Core 0 Setup: will be used for the USB mass device functions
void setup() {
  // Check consistency of RAM FS
  quark(computed_hash, msc_disk[BYTES_TO_HASH_OFFSET], BYTES_TO_HASH);

#if defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_RP2040)
  // Manual begin() is required on core without built-in support for TinyUSB such as
  // - mbed rp2040
  TinyUSB_Device_Init(0);
#endif

  // Set disk vendor id, product id and revision with string up to 8, 16, 4 characters respectively
  usb_msc.setID(USB_VENDORID, USB_PRODUCTID, USB_VERSION);

  // Set disk size (using the "fake" size)
  usb_msc.setCapacity(FAKE_DISK_BLOCK_NUM, DISK_BLOCK_SIZE);

  // Set the callback functions
  usb_msc.setReadWriteCallback(msc_read_callback, msc_write_callback, msc_flush_callback);

  // Set Lun ready (RAM disk is always ready)
  usb_msc.setUnitReady(true);

#ifdef BTN_EJECT
  pinMode(BTN_EJECT, activeState ? INPUT_PULLDOWN : INPUT_PULLUP);
  usb_msc.setReadyCallback(msc_ready_callback);
#endif

  usb_msc.begin();

  // Screen Init
  Wire.begin();
  Wire.setClock(400000L);
#if OLED_HEIGHT == 64
#if RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
#else
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
#endif
#else
#if RST_PIN >= 0
  oled.begin(&Adafruit128x32, I2C_ADDRESS, RST_PIN);
#else
  oled.begin(&Adafruit128x32, I2C_ADDRESS);
#endif
#endif

  oled.setFont(Adafruit5x7);
  oled.setScrollMode(SCROLL_MODE_AUTO);
  cls();  // Clear display

  if (memcmp(computed_hash, valid_hash, WIDTH) == 0) {
    oled.print("\n[+] Selftest: OK");
  } else {
    oled.print("\n[!] Selftest: KO");
    oled.print("\n[!] Stopping...");
    while (1) {
      delay(1000);  // Loop forever
    }
  }
}

// Core 1 Setup: will be used for the USB host functions for BADUSB detector
void setup1() {
  //while ( !Serial ) delay(10);   // wait for native usb

  pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
  pio_cfg.pin_dp = HOST_PIN_DP;
  USBHost.configure_pio_usb(1, &pio_cfg);

  // run host stack on controller (rhport) 1
  // Note: For rp2040 pico-pio-usb, calling USBHost.begin() on core1 will have most of the
  // host bit-banging processing works done in core1
  USBHost.begin(1);
}

// Main Core0 loop: managing display
void loop() {

  if (readme == true) {
    oled.print("\n[!] README (R)");
    readme = false;
  }

  if (autorun == true) {
    oled.print("\n[+] AUTORUN (R)");
    autorun = false;
  }

  if (written == true && written_reported == false) {
    oled.print("\n[!] WRITING");
    written = false;
    written_reported = true;
  }
}

// Main Core1 loop: managing USB Host
void loop1() {
  USBHost.task();
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and
// return number of copied bytes (must be multiple of block size).
// This happens only for the "real" size of disk
int32_t msc_read_callback(uint32_t lba, void* buffer, uint32_t bufsize) {

  // Check for README.TXT
  if (lba == BLOCK_README) {
    readme = true;
  }

  // Check for AUTORUN.INF
  if (lba == BLOCK_AUTORUN) {
    autorun = true;
  }

  // We are declaring a bigger size than what is actually allocated, so
  // this is protecting our memory integrity
  if (lba < DISK_BLOCK_NUM - 1) {
    uint8_t const* addr = msc_disk[lba];
    memcpy(buffer, addr, bufsize);
  }

  SerialTinyUSB.print("Read LBA: ");
  SerialTinyUSB.print(lba);
  SerialTinyUSB.print("   Size: ");
  SerialTinyUSB.println(bufsize);
  if (lba < DISK_BLOCK_NUM - 1) {
    hexDump(msc_disk[lba], MAX_DUMP_BYTES);
  }
  SerialTinyUSB.flush();

  return bufsize;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size).
// This happens only for the "real" size of disk
int32_t msc_write_callback(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {

  // This check for writing of space. The LBA > 10 is set to avoid some
  // false positives, in particular on Windows Systems
  if (lba > 10) {
    written = true;
  }
  // We are declaring a bigger size than what is actually allocated, so
  // this is protecting our memory integrity
  if (lba < DISK_BLOCK_NUM - 1) {
    uint8_t* addr = msc_disk[lba];
    memcpy(addr, buffer, bufsize);
  }

  SerialTinyUSB.print("Write LBA: ");
  SerialTinyUSB.print(lba);
  SerialTinyUSB.print("   Size: ");
  SerialTinyUSB.println(bufsize);
  if (lba < DISK_BLOCK_NUM - 1) {
    hexDump(msc_disk[lba], MAX_DUMP_BYTES);
  }
  SerialTinyUSB.flush();

  return bufsize;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void msc_flush_callback(void) {
  // Nothing to do
}

#ifdef BTN_EJECT
// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool msc_ready_callback(void) {
  // button not active --> medium ready
  return digitalRead(BTN_EJECT) != activeState;
}
#endif

// Clear display
void cls(void) {
  oled.clear();
  oled.print(VERSION);
  oled.print("\n----------------");
}

// HexDump
void hexDump(unsigned char* data, size_t size) {
  char asciitab[17];
  size_t i, j;
  asciitab[16] = '\0';

  for (i = 0; i < size; ++i) {

    SerialTinyUSB.print(data[i] >> 4, HEX);
    SerialTinyUSB.print(data[i] & 0x0F, HEX);

    if ((data)[i] >= ' ' && (data)[i] <= '~') {
      asciitab[i % 16] = (data)[i];
    } else {
      asciitab[i % 16] = '.';
    }
    if ((i + 1) % 8 == 0 || i + 1 == size) {
      SerialTinyUSB.print(" ");
      if ((i + 1) % 16 == 0) {
        SerialTinyUSB.println(asciitab);
      } else if (i + 1 == size) {
        asciitab[(i + 1) % 16] = '\0';
        if ((i + 1) % 16 <= 8) {
          SerialTinyUSB.print(" ");
        }
        for (j = (i + 1) % 16; j < 16; ++j) {
          SerialTinyUSB.print("   ");
        }
        SerialTinyUSB.print("|  ");
        SerialTinyUSB.println(asciitab);
      }
    }
  }
  SerialTinyUSB.println();
}

//
// BADUSB detector section
//

static uint8_t const keycode2ascii[128][2] = { HID_KEYCODE_TO_ASCII };

// Invoked when device with hid interface is mounted
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
  uint16_t vid, pid;
  const char* protocol_str[] = { "None", "Keyboard", "Mouse" };

  // Read the HID protocol
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  tuh_vid_pid_get(dev_addr, &vid, &pid);

  oled.print("\n[!!] HID Device");

  SerialTinyUSB.printf("HID device address = %d, instance = %d mounted\r\n", dev_addr, instance);
  SerialTinyUSB.printf("VID = %04x, PID = %04x\r\n", vid, pid);
  SerialTinyUSB.printf("HID Interface Protocol = %s\r\n", protocol_str[itf_protocol]);

  if (!tuh_hid_receive_report(dev_addr, instance)) {
    SerialTinyUSB.printf("Error: cannot request to receive report\r\n");
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
  SerialTinyUSB.printf("HID device address = %d, instance = %d unmounted\r\n", dev_addr, instance);
}

// Invoked when received report from device
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {

  static bool kbd_printed = false;
  static bool mouse_printed = false;

  oled.print("\n[!!] HID Sending data");

  // Read the HID protocol
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  switch (itf_protocol) {
    case HID_ITF_PROTOCOL_KEYBOARD:
      if (kbd_printed == false) {
        SerialTinyUSB.println("HID received keyboard report");
        kbd_printed = true;
        mouse_printed = false;
      }
      process_kbd_report((hid_keyboard_report_t const*)report);
      break;

    case HID_ITF_PROTOCOL_MOUSE:
      if (kbd_printed == false) {
        SerialTinyUSB.println("HID receive mouse report");
        mouse_printed = true;
        kbd_printed = false;
      }
      process_mouse_report((hid_mouse_report_t const*)report);
      break;

    default:
      // Generic report: for the time being we use kbd for this as well
      process_kbd_report((hid_keyboard_report_t const*)report);
      break;
  }

  if (!tuh_hid_receive_report(dev_addr, instance)) {
    SerialTinyUSB.println("Error: cannot request to receive report");
  }
}

static inline bool find_key_in_report(hid_keyboard_report_t const* report, uint8_t keycode) {
  for (uint8_t i = 0; i < 6; i++) {
    if (report->keycode[i] == keycode) return true;
  }

  return false;
}

static void process_kbd_report(hid_keyboard_report_t const* report) {
  // Previous report to check key released
  static hid_keyboard_report_t prev_report = { 0, 0, { 0 } };

  for (uint8_t i = 0; i < 6; i++) {
    if (report->keycode[i]) {
      if (find_key_in_report(&prev_report, report->keycode[i])) {
        // Exist in previous report means the current key is holding
      } else {
        // Not existed in previous report means the current key is pressed

        // Check for modifiers. It looks that in specific cases, they are not correctly recognized (probably
        // for timing issues in fast input)
        bool const is_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
        uint8_t ch = keycode2ascii[report->keycode[i]][is_shift ? 1 : 0];

        bool const is_gui = report->modifier & (KEYBOARD_MODIFIER_LEFTGUI | KEYBOARD_MODIFIER_RIGHTGUI);
        if (is_gui == true) SerialTinyUSB.printf("GUI+");

        bool const is_alt = report->modifier & (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT);
        if (is_alt == true) SerialTinyUSB.printf("ALT+");

        // Check for "special" keys
        check_special_key(report->keycode[i]);

        // Finally, print out the decoded char
        SerialTinyUSB.printf("%c", ch);
        if (ch == '\r') SerialTinyUSB.print("\n");  // New line for enter

        fflush(stdout);  // flush right away, else nanolib will wait for newline
      }
    }
  }

  prev_report = *report;
}

static void check_special_key(uint8_t code) {

  if (code == HID_KEY_ARROW_RIGHT) SerialTinyUSB.print("<ARROWRIGHT>");
  if (code == HID_KEY_ARROW_LEFT) SerialTinyUSB.print("<ARROWLEFT>");
  if (code == HID_KEY_ARROW_DOWN) SerialTinyUSB.print("<ARROWDOWN>");
  if (code == HID_KEY_ARROW_UP) SerialTinyUSB.print("<ARROWUP>");
  if (code == HID_KEY_HOME) SerialTinyUSB.print("<HOME>");
  if (code == HID_KEY_KEYPAD_1) SerialTinyUSB.print("<KEYPAD_1>");
  if (code == HID_KEY_KEYPAD_2) SerialTinyUSB.print("<KEYPAD_2>");
  if (code == HID_KEY_KEYPAD_3) SerialTinyUSB.print("<KEYPAD_3>");
  if (code == HID_KEY_KEYPAD_4) SerialTinyUSB.print("<KEYPAD_4>");
  if (code == HID_KEY_KEYPAD_5) SerialTinyUSB.print("<KEYPAD_5>");
  if (code == HID_KEY_KEYPAD_6) SerialTinyUSB.print("<KEYPAD_6>");
  if (code == HID_KEY_KEYPAD_7) SerialTinyUSB.print("<KEYPAD_7>");
  if (code == HID_KEY_KEYPAD_8) SerialTinyUSB.print("<KEYPAD_8>");
  if (code == HID_KEY_KEYPAD_9) SerialTinyUSB.print("<KEYPAD_9>");
  if (code == HID_KEY_KEYPAD_0) SerialTinyUSB.print("<KEYPAD_0>");
  if (code == HID_KEY_F1) SerialTinyUSB.print("<F1>");
  if (code == HID_KEY_F2) SerialTinyUSB.print("<F2>");
  if (code == HID_KEY_F3) SerialTinyUSB.print("<F3>");
  if (code == HID_KEY_F4) SerialTinyUSB.print("<F4>");
  if (code == HID_KEY_F5) SerialTinyUSB.print("<F5>");
  if (code == HID_KEY_F6) SerialTinyUSB.print("<F6>");
  if (code == HID_KEY_F7) SerialTinyUSB.print("<F7>");
  if (code == HID_KEY_F8) SerialTinyUSB.print("<F8>");
  if (code == HID_KEY_F9) SerialTinyUSB.print("<F9>");
  if (code == HID_KEY_F10) SerialTinyUSB.print("<F10>");
  if (code == HID_KEY_F11) SerialTinyUSB.print("<F11>");
  if (code == HID_KEY_F12) SerialTinyUSB.print("<F12>");
  if (code == HID_KEY_PRINT_SCREEN) SerialTinyUSB.print("<PRNT>");
  if (code == HID_KEY_SCROLL_LOCK) SerialTinyUSB.print("<SCRLL>");
  if (code == HID_KEY_PAUSE) SerialTinyUSB.print("<PAUSE>");
  if (code == HID_KEY_INSERT) SerialTinyUSB.print("<INSERT>");
  if (code == HID_KEY_PAGE_UP) SerialTinyUSB.print("<PAGEUP>");
  if (code == HID_KEY_DELETE) SerialTinyUSB.print("<DEL>");
  if (code == HID_KEY_END) SerialTinyUSB.print("<END>");
  if (code == HID_KEY_PAGE_DOWN) SerialTinyUSB.print("<PAGEDOWN>");
  if (code == HID_KEY_NUM_LOCK) SerialTinyUSB.print("<ARROWRIGHT>");
  if (code == HID_KEY_KEYPAD_DIVIDE) SerialTinyUSB.print("<KEYPAD_DIV>");
  if (code == HID_KEY_KEYPAD_MULTIPLY) SerialTinyUSB.print("<KEYPAD_MUL>");
  if (code == HID_KEY_KEYPAD_SUBTRACT) SerialTinyUSB.print("<KEYPAD_SUB>");
  if (code == HID_KEY_KEYPAD_ADD) SerialTinyUSB.print("<KEYPAD_ADD>");
  if (code == HID_KEY_KEYPAD_DECIMAL) SerialTinyUSB.print("<KEYPAD_DECIMAL>");
}

static void process_mouse_report(hid_mouse_report_t const* report) {
  static hid_mouse_report_t prev_report = { 0 };

  //------------- button state  -------------//
  uint8_t button_changed_mask = report->buttons ^ prev_report.buttons;
  if (button_changed_mask & report->buttons) {
    SerialTinyUSB.printf("MOUSE: %c%c%c ",
                         report->buttons & MOUSE_BUTTON_LEFT ? 'L' : '-',
                         report->buttons & MOUSE_BUTTON_MIDDLE ? 'M' : '-',
                         report->buttons & MOUSE_BUTTON_RIGHT ? 'R' : '-');
  }

  cursor_movement(report->x, report->y, report->wheel);
}

void cursor_movement(int8_t x, int8_t y, int8_t wheel) {
  SerialTinyUSB.printf("(%d %d %d)\r\n", x, y, wheel);
}

// END of BADUSB detector section