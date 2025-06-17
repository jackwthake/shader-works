#include <Arduino.h>

#include <inttypes.h>

#include <SPI.h>
#include <SdFat_Adafruit_Fork.h>
#include <Adafruit_SPIFlash.h> // SPI Flash library for QSPI flash memory

#include "ff.h"
#include "diskio.h"

#include "files.h"

Adafruit_FlashTransport_QSPI flash_transport;
Adafruit_SPIFlash flash(&flash_transport);
FatVolume fatfs;


/**
 * Writes a file from the files array to the onboard flash memory.
 * @param idx The index of the file to write in the files array.
 * @return true if the file was written successfully, false otherwise.
 */
static bool write_file_to_flash(unsigned idx) {
  if (idx >= num_files) {
    Serial.println("Invalid file index.");
    return false;
  }

  File32 file = fatfs.open(files[idx].name, FILE_WRITE);
  if (!file) {
    Serial.printf("Error opening file %s for writing.\n", files[idx].name);
    return false;
  }

  Serial.printf("Writing to file %s...  [", files[idx].name);
  file.write(files[idx].data, files[idx].size);
  file.close();
  Serial.println(" OK ]\n");
  
  return true;
}


/**
 * Writes all files defined in the files array to the onboard flash memory.
 * This function iterates through each file and calls write_file_to_flash for each one.
 */
static void write_files_to_flash() {
  Serial.println("Writing files to flash...");

  for (unsigned i = 0; i < num_files; i++) {
    if (!write_file_to_flash(i)) {
      Serial.printf("Failed to write file %s.\n", files[i].name);
      return;
    }
  }

  Serial.println("All files written successfully.");
  return;
}


/**
 * Formats the onboard flash memory as a FAT12 filesystem.
 * This function creates a new FAT12 filesystem, sets the disk label,
 * and mounts the filesystem to apply the label.
 */
void format_fat12(void) {
  uint8_t workbuf[4096];
  FATFS _fatfs;

  // Make filesystem.
  FRESULT r = f_mkfs("", FM_FAT, 0, workbuf, sizeof(workbuf));
  if (r != FR_OK) {
    Serial.print(F("Error, f_mkfs failed with error code: "));
    Serial.println(r, DEC);
    while (1)
      yield();
  }

  // mount to set disk label
  r = f_mount(&_fatfs, "0:", 1);
  if (r != FR_OK) {
    Serial.print(F("Error, f_mount failed with error code: "));
    Serial.println(r, DEC);
    for (;;);
  }

  // unmount
  f_unmount("0:");

  // sync to make sure all data is written to flash
  flash.syncBlocks();

  Serial.println("FAT12 filesystem formatted successfully.");
}


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Initializing onboard flash...");

  // Init internal flash
  flash.begin();

  Serial.println("Reformatting flash as FAT12...");
  format_fat12();

  Serial.println("Initialization done.");

  if (!fatfs.begin(&flash)) {
    Serial.println("Failed to mount FAT12 filesystem.");
    for (;;);
  }

  // File system setup, write files to flash
  write_files_to_flash();
}

void loop() {

}

//--------------------------------------------------------------------+
// fatfs diskio
//--------------------------------------------------------------------+
extern "C" {

DSTATUS disk_status(BYTE pdrv) {
  (void)pdrv;
  return 0;
}

DSTATUS disk_initialize(BYTE pdrv) {
  (void)pdrv;
  return 0;
}

DRESULT disk_read(BYTE pdrv,  /* Physical drive nmuber to identify the drive */
                  BYTE *buff, /* Data buffer to store read data */
                  DWORD sector, /* Start sector in LBA */
                  UINT count    /* Number of sectors to read */
) {
  (void)pdrv;
  return flash.readBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, /* Physical drive nmuber to identify the drive */
                   const BYTE *buff, /* Data to be written */
                   DWORD sector,     /* Start sector in LBA */
                   UINT count        /* Number of sectors to write */
) {
  (void)pdrv;
  return flash.writeBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, /* Physical drive nmuber (0..) */
                   BYTE cmd,  /* Control code */
                   void *buff /* Buffer to send/receive control data */
) {
  (void)pdrv;

  switch (cmd) {
  case CTRL_SYNC:
    flash.syncBlocks();
    return RES_OK;

  case GET_SECTOR_COUNT:
    *((DWORD *)buff) = flash.size() / 512;
    return RES_OK;

  case GET_SECTOR_SIZE:
    *((WORD *)buff) = 512;
    return RES_OK;

  case GET_BLOCK_SIZE:
    *((DWORD *)buff) = 8; // erase block size in units of sector size
    return RES_OK;

  default:
    return RES_PARERR;
  }
}
} // extern "C"