#
# To Build:
#       docker build -t usbvalve/arduino-cli .
#
# To Run:
#       docker run --rm --name usbvalve -v $PWD:/mnt usbvalve/arduino-cli /mnt/USBvalve
#

FROM ubuntu:22.04
WORKDIR /app

# OS setup
RUN apt-get update -y \ 
    && apt-get install -y git wget python3 \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# arduino-cli setup
RUN cd /app \
    && git clone --recursive https://github.com/arduino/arduino-cli.git \
    && cd arduino-cli \
    && ./install.sh \
    && export PATH=$PATH:/app/arduino-cli/bin \
    && arduino-cli --additional-urls https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json core search 2040 \
    && arduino-cli --additional-urls https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json core install rp2040:rp2040 \
    && arduino-cli lib install "Adafruit TinyUSB Library@3.3.1" \
    && arduino-cli lib install "Adafruit SSD1306@2.5.10" \
    && arduino-cli lib install "Pico PIO USB@0.5.3" \
    && arduino-cli lib install "XxHash_arduino@2.1.0" \
    && arduino-cli lib install "GFX Library for Arduino@1.4.7"

# Compilation setup
RUN echo  "#!/bin/bash" > /app/entrypoint.sh \
    && echo  "export PATH=\$PATH:/app/arduino-cli/bin" >> /app/entrypoint.sh \
    && echo  "arduino-cli compile --fqbn rp2040:rp2040:rpipico --board-options \"usbstack=tinyusb\" --board-options \"freq=120\" --output-dir \"/mnt/USBvalve_out\" \"\$1\"" >> /app/entrypoint.sh \
    && chmod +x /app/entrypoint.sh

ENTRYPOINT ["/app/entrypoint.sh"]
