#include <Arduino.h>

#include <inttypes.h>

#include <SPI.h>
#include <SdFat_Adafruit_Fork.h>
#include <Adafruit_SPIFlash.h> // SPI Flash library for QSPI flash memory


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


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Initializing Filesystem for onboard flash...");

  // Init internal flash
  flash.begin();

  // Open file system on the flash
  if (!fatfs.begin(&flash)) {
    Serial.println("Error: filesystem does not exist.");
    //TODO: format the flash instead of halting the program
    for(;;);
  }

  Serial.println("Initialization done.");

  // File system setup, write files to flash
  write_files_to_flash();
}

void loop() {

}