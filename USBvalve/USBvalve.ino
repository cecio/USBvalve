/*
  USBvalve
  
  written by Cesare Pizzi
  This project extensively reuse code doen by Adafruit. Please support them!
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

#include "Adafruit_TinyUSB.h"
#include "SSD1306AsciiWire.h"

// Define vars for OLED screen
#define I2C_ADDRESS 0x3C  // 0X3C+SA0 - 0x3C or 0x3D
#define RST_PIN -1        // Define proper RST_PIN if required.
#define OLED_HEIGHT 64    // 64 or 32 depending on the OLED
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
#define VERSION "USBvalve - 0.7.0B"
boolean readme = false;
boolean autorun = false;
boolean written = false;
boolean written_reported = false;
int x = 2;

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

// Core 0 Setup
void setup() {
  // Check consistency of RAM FS
  quark(computed_hash, msc_disk[BYTES_TO_HASH_OFFSET], BYTES_TO_HASH);

#if defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_RP2040)
  // Manual begin() is required on core without built-in support for TinyUSB such as
  // - mbed rp2040
  TinyUSB_Device_Init(0);
#endif

  // Set disk vendor id, product id and revision with string up to 8, 16, 4 characters respectively
  usb_msc.setID("Watchdog", "USBvalve", "0.1");

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
  cls();  // Clear display

  if (memcmp(computed_hash, valid_hash, WIDTH) == 0) {
    oled.println("[+] Selftest: OK");
    x++;
  } else {
    oled.println("[!] Selftest: KO");
    oled.println("[!] Stopping...");
    while (1) {
      delay(1000);  // Loop forever
    }
  }
}

// Main Core0 loop, managing display
void loop() {

  if (readme == true) {
    if (x == OLED_LINES) cls();
    oled.println("[!] README (R)");
    x++;
    readme = false;
  }

  if (autorun == true) {
    if (x == OLED_LINES) cls();
    oled.println("[+] AUTORUN (R)");
    x++;
    autorun = false;
  }

  if (written == true && written_reported == false) {
    if (x == OLED_LINES) cls();
    oled.println("[!] WRITING");
    x++;
    written = false;
    written_reported = true;
  }
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
  oled.println(VERSION);
  oled.println("----------------");
  x = 2;
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