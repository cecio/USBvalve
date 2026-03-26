/*
 * USBvalve - Pure Pico SDK Implementation
 *
 * USB security tool: acts as USB Mass Storage Device (Core 0) while
 * simultaneously running USB Host via PIO USB (Core 1) to detect
 * BadUSB/HID attacks.
 *
 * Copyright (c) Cesare Pizzi
 * MIT License
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "hardware/clocks.h"
#include "tusb.h"
#include "usb_config.h"
#include "usb_device.h"
#include "usb_host.h"
#include "display.h"
#include "serial_output.h"

#ifdef USE_BOOTSEL
#include "pico/bootrom.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#endif

//--------------------------------------------------------------------
// Shared state (cross-core, volatile. ARM word writes are atomic)
//--------------------------------------------------------------------
volatile bool flag_readme = false;
volatile bool flag_autorun = false;
volatile bool flag_written = false;
volatile bool flag_deleted = false;
volatile bool flag_written_reported = false;
volatile bool flag_deleted_reported = false;
volatile bool flag_hid_sent = false;
volatile bool flag_hid_reported = false;
volatile bool flag_hid_mounted = false;
volatile bool flag_msc_mounted = false;
volatile bool flag_cdc_mounted = false;
volatile uint32_t hid_event_num = 0;

//--------------------------------------------------------------------
// Software reset via watchdog
//--------------------------------------------------------------------
static void swreset(void) {
    watchdog_enable(1500, 1);
    while (1) {
        tight_loop_contents();
    }
}

#ifdef USE_BOOTSEL
//--------------------------------------------------------------------
// BOOTSEL button check (multi-core safe)
//
// WARNING: Reading BOOTSEL temporarily disconnects flash (QSPI CS
// override), which crashes any core executing from flash. This
// disrupts PIO USB on Core 1 and causes unreliable Low Speed device
// detection. Only enable this on boards without an external button.
//
// Core 1 must call multicore_lockout_victim_init() at startup.
//--------------------------------------------------------------------
static bool __no_inline_not_in_flash_func(bootsel_pressed_raw)(void) {
    const uint CS_PIN_INDEX = 1;

    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    for (volatile int i = 0; i < 1000; i++);

    bool pressed = !(sio_hw->gpio_hi_in & (1u << CS_PIN_INDEX));

    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    return pressed;
}

static bool bootsel_pressed(void) {
    multicore_lockout_start_blocking();
    uint32_t flags = save_and_disable_interrupts();
    bool pressed = bootsel_pressed_raw();
    restore_interrupts(flags);
    multicore_lockout_end_blocking();
    return pressed;
}
#endif // USE_BOOTSEL

//--------------------------------------------------------------------
// Main (Core 0)
//--------------------------------------------------------------------
int main(void) {
    // Sysclock must be a multiple of 12MHz for PIO USB.
#if defined(PICO_RP2350)
    set_sys_clock_khz(144000, true);
#else
    set_sys_clock_khz(240000, true);
#endif
    sleep_ms(10);

    // Launch Core 1 for USB host BEFORE device init.
    // Core 1 handles PIO USB host. Must start first so PIO IRQs
    // are registered on Core 1.
    multicore_reset_core1();
    multicore_launch_core1(core1_entry);

    // Init device stack on native USB (rhport 0)
    tud_init(0);

    // Run ramdisk self-test
    bool selftest_ok = usb_device_selftest();

    // Initialize display
    display_init();
    cls();

    if (selftest_ok) {
        printout("\n[+] Selftest: OK");
    } else {
        printout("\n[!] Selftest: KO");
        printout("\n[!] Stopping...");
        while (1) {
            sleep_ms(1000);
        }
    }

#ifndef PIWATCH
    // Set up LED pin (PIWATCH uses GPIO 25 for backlight)
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1); // LED on = OK

    // Set up external reset button (active-low with pull-up)
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
#endif

    // Core 0 main loop
    while (true) {
        // Process TinyUSB device tasks (required for CDC serial + MSC)
        tud_task();

        // Flush CDC writes from Core 1 callbacks
        tud_cdc_write_flush();

        // Check flags and report events
        if (flag_readme) {
            printout("\n[!] README (R)");
            flag_readme = false;
#ifndef PIWATCH
            gpio_put(LED_PIN, 0);
#endif
        }

        if (flag_autorun) {
            printout("\n[+] AUTORUN (R)");
            flag_autorun = false;
        }

        if (flag_deleted && !flag_deleted_reported) {
            printout("\n[!] DELETING");
            flag_deleted = false;
            flag_deleted_reported = true;
#ifndef PIWATCH
            gpio_put(LED_PIN, 0);
#endif
        }

        if (flag_written && !flag_written_reported) {
            printout("\n[!] WRITING");
            flag_written = false;
            flag_written_reported = true;
#ifndef PIWATCH
            gpio_put(LED_PIN, 0);
#endif
        }

        if (flag_hid_mounted) {
            printout("\n[!!] HID Device");
            flag_hid_mounted = false;
#ifndef PIWATCH
            gpio_put(LED_PIN, 0);
#endif
        }

        if (flag_msc_mounted) {
            printout("\n[++] Mass Device");
            flag_msc_mounted = false;
        }

        if (flag_cdc_mounted) {
            printout("\n[++] CDC Device");
            flag_cdc_mounted = false;
        }

        if (flag_hid_sent && !flag_hid_reported) {
            printout("\n[!!] HID Sending data");
            flag_hid_sent = false;
            flag_hid_reported = true;
#ifndef PIWATCH
            gpio_put(LED_PIN, 0);
#endif
        }

#ifndef PIWATCH
        // External button on GPIO 0 (active-low, debounced)
        if (!gpio_get(BUTTON_PIN)) {
            sleep_ms(50); // debounce
            if (!gpio_get(BUTTON_PIN)) {
                uint32_t press_start = to_ms_since_boot(get_absolute_time());
                while (!gpio_get(BUTTON_PIN)) {
                    sleep_ms(10);
                }
                uint32_t press_duration = to_ms_since_boot(get_absolute_time()) - press_start;

                if (press_duration > 2000) {
                    // Long press: show HID event count
                    char outstr[32];
                    snprintf(outstr, sizeof(outstr), "\n[+] HID Evt# %u", (unsigned)hid_event_num);
                    printout(outstr);
                } else {
                    // Short press: reset
                    printout("\n[+] RESETTING");
                    swreset();
                }
            }
        }
#endif // !PIWATCH

        // CDC serial commands
        // 'r'/'R' = reset, 'h'/'H' = HID event count
        if (tud_cdc_available()) {
            char buf[64];
            uint32_t count = tud_cdc_read(buf, sizeof(buf));
            for (uint32_t i = 0; i < count; i++) {
                switch (buf[i]) {
                    case 'r': case 'R':
                        printout("\n[+] RESETTING");
                        swreset();
                        break;
                    case 'h': case 'H': {
                        char outstr[32];
                        snprintf(outstr, sizeof(outstr), "\n[+] HID Evt# %u", (unsigned)hid_event_num);
                        printout(outstr);
                        break;
                    }
                    default:
                        break;
                }
            }
        }

#ifdef USE_BOOTSEL
        // BOOTSEL polling (disabled by default, disrupts Low Speed USB).
        // Only enable via -DUSE_BOOTSEL for boards without external button.
        static uint32_t last_bootsel_check = 0;
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_bootsel_check > 1500) {
            last_bootsel_check = now;
            if (bootsel_pressed()) {
                uint32_t press_start = to_ms_since_boot(get_absolute_time());
                while (bootsel_pressed()) {
                    sleep_ms(10);
                }
                uint32_t press_duration = to_ms_since_boot(get_absolute_time()) - press_start;

                if (press_duration > 2000) {
                    char outstr[32];
                    snprintf(outstr, sizeof(outstr), "\n[+] HID Evt# %u", (unsigned)hid_event_num);
                    printout(outstr);
                } else {
                    printout("\n[+] RESETTING");
                    swreset();
                }
            }
        }
#endif // USE_BOOTSEL
    }

    return 0;
}
