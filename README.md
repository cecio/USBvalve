<h1><img width="300" alt="logo, landscape, dark text, transparent background" src="https://github.com/cecio/USBvalve/blob/main/pictures/USBvalve_logo_scaled.png"></a></h1>

### *Expose USB activity on the fly*

<p float="left">
<img src="https://github.com/cecio/USBvalve/blob/main/pictures/versions.png" alt="The two models" width="30%" height="30%" />
<img src="https://github.com/cecio/USBvalve/blob/main/pictures/USBvalve_PIWATCH.png" alt="The Watch" width="26%" height="26%" />
<img src="https://github.com/cecio/USBvalve/blob/main/pictures/version1_2.png" alt="1.2" width="26%" height="26%" />
</p>

I'm sure that, like me, you were asked to put your USB drive in an *unknown* device...and then the doubt: 

what happened to my  poor dongle, behind the scene? Stealing my files? Encrypting them? Or  *just* installing a malware? With **USBvalve** you can spot this out in  seconds: built on super cheap off-the-shelf hardware you can quickly  test any USB file system activity and understand what is going on before it's too late!

With **USBvalve** you can have an immediate feedback about what happen to the drive; the screen will show you if the *fake* filesystem built on the device is accessed, read or written:

<p float="left">
<img src="https://github.com/cecio/USBvalve/blob/main/pictures/selftest.png" alt="Selftest" width="15%" height="15%" />
<img src="https://github.com/cecio/USBvalve/blob/main/pictures/readme.png" alt="Readme" width="15%" height="15%" />
</p>

and from version `0.8.0` you can also use it as USB Host to detect *BADUSB* devices:

<p float="left">
<img src="https://github.com/cecio/USBvalve/blob/main/pictures/hid.png" alt="HID" width="15%" height="15%" />
</p>

This is an example of the *BADUSB* debugger available on serial port:

<p float="left">
<a href="https://asciinema.org/a/NWfC9Mvzzpj3eZfsC7s5Dz1sJ" target="_blank"><img src="https://asciinema.org/a/NWfC9Mvzzpj3eZfsC7s5Dz1sJ.svg" width="65%" height="65%" /></a>
</p>

If you prefer videos, you can also have a look to my [Insomni'hack Presentation](https://www.youtube.com/watch?v=jy1filtQY4w)

## USBvalve Watch

Starting from version `0.15.0` a new *Pi Pico Watch* version is supported. To compile the new version you have to uncomment the `#define PIWATCH` line at the beginning of the code. The hardware is a RP2040-based 1.28-inch TFT display and watch board. You can find some more info [here](https://www.raspberrypi.com/news/how-to-build-your-own-raspberry-pi-watch/).
This is also fully compatible with the [Waveshare RP2040-LCD-1.28](https://www.waveshare.com/wiki/RP2040-LCD-1.28).

## Repository Structure

`docs`: documentation about the project, with a presentation where you can have a look to all the features

`firmware`: pre-built firmware for the Raspberry Pi Pico. You can just use these and flash them on the board. I prepared the two versions for 32 and 64 OLED versions

`PCB`: Gerber file if you want to print the custom PCB . It's not mandatory, you can use your own or build it on a breadboard

`USBvalve`: sources, if you want to modify and build the firmware by yourself

`utils`: some utilities you may use to build a custom FS

`pictures`: images and resources used in this doc

`STL`: STL files for enclosure. In `1.1` and `1.2` folders there are full enclosures (thanks to [WhistleMaster](https://github.com/WhistleMaster)). If you want something lighter to protect the LCD you can go with `USBvalve_sliding_cover.stl`.

## Build USBvalve

### Part list

If you want to build your own, you need:

- A Raspberry Pi Pico (or another RP2040 based board, like Arduino Nano RP2040)
- an I2C OLED screen 128x64 or 128x32
- (optional) a **USBvalve** PCB or a breadboard
- (optional) a 3D printed spacer to isolate the screen from the board (https://www.thingiverse.com/thing:4748043), but you can use a piece of electrical tape instead

### Building instructions

Almost all the job is done directly on the board by the software, so you just need to arrange the connection with the OLED for output.

Starting from version 0.8.0 of the firmware, **USBvalve** can detect HID devices (used to detect *BADUSB*). This require an additional USB port behaving as Host. If you are not interested in this, you can use the old instructions [in docs folder](https://github.com/cecio/USBvalve/blob/main/docs/BUILDING-1.1.md) and use PCB version `1.1`. Otherwise go ahead with PCB version `1.2` (we have version for USB-A or USB-B, see folder).

#### With USBvalve PCB

<p float="left">
  <img src="https://github.com/cecio/USBvalve/blob/main/pictures/USB_valve_1-2_front.png" width="25%" height="25%" />
  <img src="https://github.com/cecio/USBvalve/blob/main/pictures/USB_valve_1-2_back.png" width="24%" height="24%" /> 
</p>


- solder a USB female port in `USBH` area. This is for version `A`, but there is a version for USB `Micro-B` as well if you prefer
- place the Raspberry Pi Pico on the silk screen on the front 
- you don't need to solder all the PINs. Just the following:
  - D4 and D5 (left side)
  - D14 and D15 (left side)
  - GND (right side, third pin from the top)
  - GND (right side, third pin from the bottom)
  - 3v3_OUT (right side)
  - VBUS (right side)
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

If you are using a breadboard or just wiring, all you have to do is to ensure to connect the proper PINs at the OLED screen and to the Host USB port.

The mapping is the following:

- PIN6 of Pi --> OLED SDA
- PIN7 of Pi --> OLED SCL 
- PIN19 of Pi --> D+ of USB Host
- PIN20 of Pi --> D- of USB Host
- PIN23 (GND) of Pi --> GND of USB Host
- PIN38 (GND) of Pi --> OLED GND
- PIN36 (3V3OUT) of Pi --> OLED VCC
- PIN40 (VBUS) of Pi --> VCC of USB Host

If you want to use the DEBUG functions, you can also place a header on the 3 SWD PINs at the bottom of the board.

### Flash Firmware

To flash the firmware, follow these steps:

- Connect the Raspberry Pi Pico with the USB cable, by keeping the *BOOTSEL* button pressed (the big white button on the board)
- release the button
- you will see a new drive on the system, named `RPI-RP2` (in Linux envs you may have to manually mount it)
- copy the proper firmware file (with extension `uf2`) in the folder, depending on the OLED you used
- wait few seconds until the mounted folder disappear

It's done!

### Anti-Detection

I don't know if it will ever be the case, but you may want to customize the firmware in order to avoid detection done by *USBvalve-aware* malware :-)

I grouped most of the variables you may want to modify in this section ([see Dockerfile below for rebuilding](https://github.com/cecio/USBvalve#dockerfile))

```C
// Anti-Detection settings.
//
// Set USB IDs strings and numbers, to avoid possible detections.
// Remember that you can cusotmize FAKE_DISK_BLOCK_NUM as well
// for the same reason. Also DISK_LABEL in ramdisk.h can be changed.
//
// You can see here for inspiration: https://the-sz.com/products/usbid/
//
// Example:
//             0x0951 0x16D5    VENDORID_STR: Kingston   PRODUCTID_STR: DataTraveler
//
#define USB_VENDORID 0x0951               // This override the Pi Pico default 0x2E8A
#define USB_PRODUCTID 0x16D5              // This override the Pi Pico default 0x000A
#define USB_DESCRIPTOR "DataTraveler"     // This override the Pi Pico default "Pico"
#define USB_MANUF "Kingston"              // This override the Pi Pico default "Raspberry Pi"
#define USB_SERIAL "123456789A"           // This override the Pi Pico default. Disabled by default. \
                                          // See "setSerialDescriptor" in setup() if needed
#define USB_VENDORID_STR "Kingston"       // Up to 8 chars
#define USB_PRODUCTID_STR "DataTraveler"  // Up to 16 chars
#define USB_VERSION_STR "1.0"             // Up to 4 chars
```


### Building your firmware

Obviously you can also build your own firmware. To build the *standard* one I used:

- Arduino IDE `2.3.2`
- `Adafruit TinyUSB Library` version `3.1.1`, `Pico-PIO-USB` version `0.5.2`, Board `Raspberry Pi RP2040 (3.7.2)` setting Tools=>CPU Speed at `120MHz` and Tools=>USB Stack to `Adafruit TinyUSB`
- `ssd1306` OLED library version `1.8.3`

If you want to re-create a new fake filesystem, you may want to have a look to the `utils` folder, where I placed some utilities to build a new one.

#### Dockerfile

If you want to build your own firmware, after you customized it, I provide a `Dockerfile` which builds a complete **Arduino** environment and compile the firmware. Enter the following commands in the main `USBvalve` folder:

```
docker build -t usbvalve/arduino-cli .
docker run --rm --name usbvalve -v $PWD:/mnt usbvalve/arduino-cli /mnt/USBvalve 
```

The firmware will be placed with extension `uf2` in folder `USBvalve_out`.

### Contribute

If you have ideas or improvements in your mind, I encourage you to open an issue so that we can improve the project together! Thanks!

### Support

If you have question or need support you can open an `Issue` here or reach me out on Twitter/X [@red5heep](https://twitter.com/red5heep)

## SAFETY WARNING

I've received a lot of questions about **USBvalve** and *USB killer devices*. **USBvalve** is not built to test these devices, it has not any kind of insulation or protection, so if you have the suspect you are dealing with one of these devices, test it with something else, NOT with **USBvalve** or you may damage the device, yourself or objects near to you.
