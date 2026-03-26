#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
FW_DIR="$SCRIPT_DIR/firmware"
BUILD_DIR="$SCRIPT_DIR/build"

# Extract version from usb_config.h
FW_VERSION=$(grep '#define VERSION' "$SCRIPT_DIR/src/usb_config.h" | sed 's/.*"\(.*\)"/\1/' | sed 's/USBvalve - //')

echo "Firmware version: $FW_VERSION"
mkdir -p "$FW_DIR"

build() {
    local name="$1"
    shift
    echo "=========================================="
    echo "Building: $name"
    echo "CMake args: $@"
    echo "=========================================="
    rm -rf "$BUILD_DIR"
    mkdir "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake "$@" ..
    make -j$(nproc)
    cp src/USBvalve.uf2 "$FW_DIR/${name}.uf2"
    echo "-> $FW_DIR/${name}.uf2"
    echo ""
}

build "USBvalve-${FW_VERSION}-pico1-oled32" \
    -DPICO_BOARD=pico -DOLED_HEIGHT=32

build "USBvalve-${FW_VERSION}-pico1-oled64" \
    -DPICO_BOARD=pico -DOLED_HEIGHT=64

build "USBvalve-${FW_VERSION}-pico1-oled32-bootsel" \
    -DPICO_BOARD=pico -DOLED_HEIGHT=32 -DUSE_BOOTSEL=1

build "USBvalve-${FW_VERSION}-pico1-oled64-bootsel" \
    -DPICO_BOARD=pico -DOLED_HEIGHT=64 -DUSE_BOOTSEL=1

build "USBvalve-${FW_VERSION}-pico2-oled32" \
    -DPICO_BOARD=pico2 -DOLED_HEIGHT=32

build "USBvalve-${FW_VERSION}-pico2-oled64" \
    -DPICO_BOARD=pico2 -DOLED_HEIGHT=64

build "USBvalve-${FW_VERSION}-pico2-oled32-bootsel" \
    -DPICO_BOARD=pico2 -DOLED_HEIGHT=32 -DUSE_BOOTSEL=1

build "USBvalve-${FW_VERSION}-pico2-oled64-bootsel" \
    -DPICO_BOARD=pico2 -DOLED_HEIGHT=64 -DUSE_BOOTSEL=1

build "USBvalve-${FW_VERSION}-pico1-piwatch-bootsel" \
    -DPICO_BOARD=pico -DPIWATCH=1 -DUSE_BOOTSEL=1

echo "=========================================="
echo "All builds complete. Firmware files:"
ls -la "$FW_DIR"/*.uf2
