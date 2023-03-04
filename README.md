# USBvalve
*Expose USB activity on the fly*

<img src="https://github.com/cecio/USBvalve/blob/main/pictures/versions.png" alt="The two models" width="30%" height="30%" />

I'm sure that, like me, you were asked to put your USB drive in an *unknown* device...and then the doubt: 

what happened to my  poor dongle, behind the scene? Stealing my files? Encrypting them? Or  *just* installing a malware? With **USBvalve** you can spot this out in  seconds: built on super cheap off-the-shelf hardware you can quickly  test any USB file system activity and understand what is going on before it's too late!

With **USBvalve** you can have an immediate feedback about what happen to the drive; the screen will show you if the *fake* filesystem built on the device is accessed, read or written:

<p float="left">
<img src="https://github.com/cecio/USBvalve/blob/main/pictures/selftest.png" alt="Selftest" width="15%" height="15%" />
<img src="https://github.com/cecio/USBvalve/blob/main/pictures/readme.png" alt="Readme" width="15%" height="15%" />
</p>

## Repository Structure

`docs`: documentation about the project, with a presentation where you can have a look to all the features

`firmware`: pre-built firmware for the Raspberry Pi Pico. You can just use these and flash them on the board. I prepared the two versions for 32 and 64 OLED versions

`PCB`: Gerber file if you want to print the custom PCB . It's not mandatory, you can use your own or build it on a breadboard

`USBvalve`: sources, if you want to modify and build the firmware by yourself

`utils`: some utilities you may use to build a custom FS

`pitcures`: images and resources used in this doc

## Build USBvalve

### Part list

If you want to build your own, you need:

- A Raspberry Pi Pico (or another RP2040 based board, like Arduino Nano RP2040)
- an I2C OLED screen 128x64 or 128x32
- (optional) a **USBvalve** PCB or a breadboard
- (optional) a 3D printed spacer to isolate the screen from the board (https://www.thingiverse.com/thing:4748043), but you can use a piece of electrical tape instead

### Building instructions

Almost all the job is done directly on the board by the software, so you just need to arrange the connection with the OLED for output

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

### Building your firmware

Obviously you can also build your own firmware. To build the *standard* one I used:

- Arduino IDE 2.0.4
- as board I used `Raspberry Pi Pico - Arduino MBED OS RP2040` version `4.0.2`
- `Adafruit TinyUSB Library` version `1.14.4`. Newer versions are not working because the RPI SDK of the board is stick to an older version.  May be migrate the entire project directly on Raspberry Pi Pico SDK is the solution here.
- `ssd1306` OLED library version `1.8.3`

If you want to re-create a new fake filesystem, you may want to have a look to the `utils` folder, where I placed some utilities to build a new one.

**NOTE**: if you have ideas or improvements in your mind, I encourage you to open an issue so that we can improve the project together! Thanks!

