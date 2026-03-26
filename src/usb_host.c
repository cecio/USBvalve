/*
 * USBvalve - USB Host (Core 1)
 *
 * PIO USB host for detecting HID/MSC/CDC devices.
 * Runs entirely on Core 1.
 *
 * Uses tud_cdc_write() for serial output (same as working example).
 * Core 0 handles display updates via flags.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "pio_usb.h"
#include "tusb.h"
#include "usb_config.h"
#include "usb_host.h"
#include "hid_handler.h"

// Shared flags (defined in main.c)
extern volatile bool flag_hid_sent;
extern volatile bool flag_hid_reported;
extern volatile bool flag_hid_mounted;
extern volatile bool flag_msc_mounted;
extern volatile bool flag_cdc_mounted;
extern volatile uint32_t hid_event_num;

//--------------------------------------------------------------------
// Host Init
//--------------------------------------------------------------------
void usb_host_init(void) {
    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = HOST_PIN_DP;

    tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
    tuh_init(1);
}

void core1_entry(void) {
    sleep_ms(10);

    // Allow Core 0 to temporarily pause us (for safe BOOTSEL reading).
    // The lockout handler runs from RAM, so flash can be disconnected.
    multicore_lockout_victim_init();

    usb_host_init();

    while (true) {
        tuh_task();
    }
}

//--------------------------------------------------------------------
// TinyUSB Host HID Callbacks
// Uses tud_cdc_write() for serial (same pattern as working example).
// No I2C / display calls, those happen on Core 0 via flags.
//--------------------------------------------------------------------

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                       uint8_t const* desc_report, uint16_t desc_len) {
    (void)desc_report;
    (void)desc_len;

    const char* protocol_str[] = {"None", "Keyboard", "Mouse"};
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    char tempbuf[256];
    int count = sprintf(tempbuf, "[%04x:%04x][%u] HID Interface%u, Protocol = %s\r\n",
                        vid, pid, dev_addr, instance, protocol_str[itf_protocol]);
    tud_cdc_write(tempbuf, count);
    tud_cdc_write_flush();

    flag_hid_mounted = true;

    // Request reports for all protocols, some keyboards report as "None"
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        tud_cdc_write_str("Error: cannot request report\r\n");
    }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    char tempbuf[64];
    int count = sprintf(tempbuf, "[%u] HID Interface%u unmounted\r\n", dev_addr, instance);
    tud_cdc_write(tempbuf, count);
    tud_cdc_write_flush();

    flag_hid_sent = false;
    flag_hid_reported = false;
    hid_event_num = 0;
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                 uint8_t const* report, uint16_t len) {
    flag_hid_sent = true;

    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

    switch (itf_protocol) {
        case HID_ITF_PROTOCOL_KEYBOARD:
            process_kbd_report(report, len);
            hid_event_num++;
            break;

        case HID_ITF_PROTOCOL_MOUSE:
            process_mouse_report((hid_mouse_report_t const*)report);
            hid_event_num++;
            break;

        default:
            // Generic HID (protocol "None"): treat as keyboard
            process_kbd_report(report, len);
            hid_event_num++;
            break;
    }

    if (!tuh_hid_receive_report(dev_addr, instance)) {
        tud_cdc_write_str("Error: cannot request report\r\n");
    }
}

//--------------------------------------------------------------------
// TinyUSB Host MSC/CDC Callbacks
//--------------------------------------------------------------------

void tuh_msc_mount_cb(uint8_t dev_addr) {
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    char tempbuf[64];
    int count = sprintf(tempbuf, "[%04x:%04x][%u] MSC mounted\r\n", vid, pid, dev_addr);
    tud_cdc_write(tempbuf, count);
    tud_cdc_write_flush();

    flag_msc_mounted = true;
}

void tuh_msc_umount_cb(uint8_t dev_addr) {
    char tempbuf[64];
    int count = sprintf(tempbuf, "[%u] MSC unmounted\r\n", dev_addr);
    tud_cdc_write(tempbuf, count);
    tud_cdc_write_flush();
}

void tuh_cdc_mount_cb(uint8_t idx) {
    char tempbuf[64];
    int count = sprintf(tempbuf, "[%u] CDC mounted\r\n", idx);
    tud_cdc_write(tempbuf, count);
    tud_cdc_write_flush();

    flag_cdc_mounted = true;
}

void tuh_cdc_umount_cb(uint8_t idx) {
    char tempbuf[64];
    int count = sprintf(tempbuf, "[%u] CDC unmounted\r\n", idx);
    tud_cdc_write(tempbuf, count);
    tud_cdc_write_flush();
}
