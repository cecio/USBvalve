#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------
// Common Configuration
//--------------------------------------------------------------------
#define CFG_TUSB_OS           OPT_OS_PICO

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN    __attribute__((aligned(4)))
#endif

//--------------------------------------------------------------------
// Device Configuration (rhport 0 = native USB)
//--------------------------------------------------------------------
#define CFG_TUD_ENABLED       1

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE  64
#endif

// Device classes
#define CFG_TUD_CDC           1
#define CFG_TUD_MSC           1
#define CFG_TUD_HID           0
#define CFG_TUD_MIDI          0
#define CFG_TUD_VENDOR        0

// CDC FIFO sizes
#define CFG_TUD_CDC_RX_BUFSIZE  256
#define CFG_TUD_CDC_TX_BUFSIZE  256

// MSC buffer size
#define CFG_TUD_MSC_EP_BUFSIZE  512

//--------------------------------------------------------------------
// Host Configuration (rhport 1 = PIO USB)
//--------------------------------------------------------------------
#define CFG_TUH_ENABLED       1
#define CFG_TUH_RPI_PIO_USB   1

// Host classes
#define CFG_TUH_HID           4
#define CFG_TUH_MSC           1
#define CFG_TUH_CDC           1
#define CFG_TUH_HUB           0

// Max devices
#define CFG_TUH_DEVICE_MAX    1
#define CFG_TUH_ENUMERATION_BUFSIZE 256

// HID buffer
#define CFG_TUH_HID_EPIN_BUFSIZE   64
#define CFG_TUH_HID_EPOUT_BUFSIZE  64

// CDC FIFO sizes for host
#define CFG_TUH_CDC_RX_BUFSIZE  64
#define CFG_TUH_CDC_TX_BUFSIZE  64

#ifdef __cplusplus
}
#endif

#endif // TUSB_CONFIG_H
