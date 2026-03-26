# Match the working pico-pio-usb host_hid_to_device_cdc example build pattern

set(target_name USBvalve)
add_executable(${target_name})

target_sources(${target_name} PRIVATE
    main.c
    usb_device.c
    usb_descriptors.c
    usb_host.c
    hid_handler.c
    display.c
    serial_output.c
    ../lib/xxhash/xxhash.c
    # PIO USB TinyUSB portable drivers (same as the example)
    ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c
    ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
)

target_link_options(${target_name} PRIVATE -Xlinker --print-memory-usage)
target_compile_options(${target_name} PRIVATE -Wall -Wextra)
target_compile_definitions(${target_name} PRIVATE PIO_USB_USE_TINYUSB)

# OLED display height: 32 (default) or 64
# Usage: cmake -DOLED_HEIGHT=64 ..
if(NOT DEFINED OLED_HEIGHT)
    set(OLED_HEIGHT 32)
endif()
target_compile_definitions(${target_name} PRIVATE OLED_HEIGHT=${OLED_HEIGHT})

# PIWATCH: GC9A01 round TFT display instead of SSD1306 OLED
# Usage: cmake -DPIWATCH=1 ..
if(PIWATCH)
    target_compile_definitions(${target_name} PRIVATE PIWATCH)
endif()

# Optional: enable BOOTSEL button support (disrupts Low Speed USB detection)
# Usage: cmake -DUSE_BOOTSEL=1 ..
if(USE_BOOTSEL)
    target_compile_definitions(${target_name} PRIVATE USE_BOOTSEL)
endif()

target_include_directories(${target_name} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../data
    ${CMAKE_CURRENT_LIST_DIR}/../lib/xxhash
)

target_link_libraries(${target_name} PRIVATE
    pico_stdlib
    pico_pio_usb
    tinyusb_device
    tinyusb_host
    hardware_i2c
    hardware_spi
    hardware_watchdog
)

pico_add_extra_outputs(${target_name})
