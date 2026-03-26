cmake_minimum_required(VERSION 3.13)

# Include the Pico SDK import helper
include(pico_sdk_import.cmake)

project(USBvalve C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Add pico-pio-usb library
add_subdirectory(lib/pico-pio-usb)

# Add application
add_subdirectory(src)
