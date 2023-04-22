#### With USBvalve PCB

<p float="left">
  <img src="https://github.com/cecio/USBvalve/blob/main/pictures/USB_valve_front.png" width="25%" height="25%" />
  <img src="https://github.com/cecio/USBvalve/blob/main/pictures/USB_valve_back.png" width="25%" height="25%" /> 
</p>


- place the Raspberry Pi Pico on the silk screen on the front 
- you don't need to solder all the PINs. Just the following:
  - D4 and D5 (left side)
  - GND (right side)
  - 3v3_OUT (right side)
  - the 3 DEBUG pin on the bottom: SWCLK, GND and SWDIO
- place the 3D printer spacer or a piece of tape on the parts of the OLED that my touch the Raspberry
- solder the OLED (with a header) on the 4 PIN space

Some of the OLEDs have the GND and VCC PINs swapped, so I built the PCB to be compatible with both versions: 

For example if your OLED has GND on PIN1 and VCC on PIN2 like this:

<img src="https://github.com/cecio/USBvalve/blob/main/pictures/usb_valve_oled.png" width="35%" height="35%" />

You have to place a blob of solder on these two pads on the back of the PCB:

<img src="https://github.com/cecio/USBvalve/blob/main/pictures/usb_valve_pads.png" width="15%" height="15%" />

Otherwise you should the opposite and place the solder on the other PADs:

<img src="https://github.com/cecio/USBvalve/blob/main/pictures/usb_valve_pads_2.png" width="15%" height="15%" />

#### Without USBvalve PCB

<img src="https://github.com/cecio/USBvalve/blob/main/pictures/pico-pinout.svg" alt="Pico Pi" width="85%" height="85%" />

If you are using a breadboard or just wiring, all you have to do is to ensure to connect the proper PINs at the OLED screen.

The mapping is the following:

- PIN6 of Pi --> OLED SDA
- PIN7 of Pi --> OLED SCL 
- PIN38 (GND) of Pi --> OLED GND
- PIN36 (3V3OUT) of Pi --> OLED VCC

If you want to use the DEBUG functions, you can also place a header on the 3 SWD PINs at the bottom of the board.

### Flash Firmware

To flash the firmware, follow these steps:

- Connect the Raspberry Pi Pico with the USB cable, by keeping the *BOOTSEL* button pressed (the big white button on the board)
- release the button
- you will see a new drive on the system, named `RPI-RP2` (in Linux envs you may have to manually mount it)
- copy the proper firmware file (with extension `uf2`) in the folder, depending on the OLED you used
- wait few seconds until the mounted folder disappear

It's done!

