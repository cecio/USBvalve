/*
 * USBvalve - USB Mass Storage Device (Core 0)
 *
 * Implements TinyUSB MSC callbacks for a fake FAT12 disk.
 */

#include "tusb.h"
#include "usb_config.h"
#include "usb_device.h"
#include "serial_output.h"
#include "ramdisk.h"

#include "xxhash.h"

// Shared flags (accessed from both cores via spinlock)
extern volatile bool flag_readme;
extern volatile bool flag_autorun;
extern volatile bool flag_written;
extern volatile bool flag_deleted;

//--------------------------------------------------------------------
// Init
//--------------------------------------------------------------------
void usb_device_init(void) {
    // TinyUSB device init is done via tusb_init() in main
}

bool usb_device_selftest(void) {
    // Hash the FAT directory area to verify ramdisk integrity
    // Add 11 bytes to skip the DISK_LABEL from the hashing
    XXH32_hash_t computed = XXH32(msc_disk[BYTES_TO_HASH_OFFSET] + 11, BYTES_TO_HASH, 0);
    return (computed == VALID_HASH);
}

//--------------------------------------------------------------------
// MSC Callbacks
//--------------------------------------------------------------------

// Invoked when host sends SCSI INQUIRY
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8],
                         uint8_t product_id[16], uint8_t product_rev[4]) {
    (void)lun;

    const char vid[] = USB_VENDORID_STR;
    const char pid[] = USB_PRODUCTID_STR;
    const char rev[] = USB_VERSION_STR;

    memcpy(vendor_id, vid, strlen(vid));
    memcpy(product_id, pid, strlen(pid));
    memcpy(product_rev, rev, strlen(rev));
}

// Invoked when host asks for disk capacity
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
    (void)lun;
    *block_count = FAKE_DISK_BLOCK_NUM;
    *block_size  = DISK_BLOCK_SIZE;
}

// Invoked on READ10 command
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                           void* buffer, uint32_t bufsize) {
    (void)lun;
    (void)offset;

    // Check for README.TXT access
    if (lba == BLOCK_README) {
        flag_readme = true;
    }

    // Check for AUTORUN.INF access
    if (lba == BLOCK_AUTORUN) {
        flag_autorun = true;
    }

    // Return data from ramdisk, or zeros for blocks beyond it.
    // The disk reports FAKE_DISK_BLOCK_NUM sectors but only
    // DISK_BLOCK_NUM exist in RAM — the rest must read as empty.
    if (lba < DISK_BLOCK_NUM) {
        memcpy(buffer, msc_disk[lba], bufsize);
    } else {
        memset(buffer, 0, bufsize);
    }

    serial_printf("Read LBA: %u   Size: %u\r\n", (unsigned)lba, (unsigned)bufsize);
    if (lba < DISK_BLOCK_NUM) {
        hex_dump(msc_disk[lba], MAX_DUMP_BYTES);
    }

    return (int32_t)bufsize;
}

// Invoked on WRITE10 command
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                            uint8_t* buffer, uint32_t bufsize) {
    (void)lun;
    (void)offset;

    // Check for file deletion at Block 7
    if (lba == 7) {
        if (buffer[32] == 0xE5 || buffer[64] == 0xE5 || buffer[160] == 0xE5) {
            flag_deleted = true;
        }
    }

    // Check for file writes (LBA > 10 to avoid false positives)
    if (lba > 10) {
        flag_written = true;
    }

    // Only write to real disk blocks; silently discard beyond ramdisk
    if (lba < DISK_BLOCK_NUM) {
        memcpy(msc_disk[lba], buffer, bufsize);
    }

    serial_printf("Write LBA: %u   Size: %u\r\n", (unsigned)lba, (unsigned)bufsize);
    if (lba < DISK_BLOCK_NUM) {
        hex_dump(msc_disk[lba], MAX_DUMP_BYTES);
    }

    return (int32_t)bufsize;
}

// Invoked when WRITE10 completes
void tud_msc_write10_complete_cb(uint8_t lun) {
    (void)lun;
    // Nothing to do - no persistent storage
}

// Invoked on SCSI commands that are not READ10/WRITE10
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16],
                         void* buffer, uint16_t bufsize) {
    int32_t resplen = 0;

    switch (scsi_cmd[0]) {
        case SCSI_CMD_TEST_UNIT_READY:
            // Disk is always ready
            resplen = 0;
            break;

        case SCSI_CMD_START_STOP_UNIT: {
            // Handle eject
            uint8_t const start = scsi_cmd[4] & 0x01;
            uint8_t const loej  = scsi_cmd[4] & 0x02;
            if (loej && !start) {
                // Eject requested - we just ignore it
            }
            resplen = 0;
            break;
        }

        default:
            // Unknown SCSI command - set sense data for unsupported command
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
            resplen = -1;
            break;
    }

    return resplen;
}

// Unit ready check
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    (void)lun;
    return true;  // RAM disk is always ready
}
