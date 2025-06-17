#include <Arduino.h>

#include <inttypes.h>

#include <SPI.h>
#include <Adafruit_SPIFlash.h> // SPI Flash library for QSPI flash memory


#include "files.h"

Adafruit_FlashTransport_QSPI flash_transport;
Adafruit_SPIFlash flash(&flash_transport);
FatFileSystem file_sys = FatFileSystem();

FatVolume fatfs;
File32 myFile;

char *cwd_path = new char[64]{
    '/', '\0' // Initialize with root directory
};


static bool write_file_to_flash(unsigned idx) {
  if (idx < 0 || idx >= num_files) {
    Serial.println("Invalid file index.");
    return false;
  }

  File32 file = fatfs.open(files[idx].name, FILE_WRITE);
  if (!file) {
    Serial.printf("Error opening file %s for writing.\n", files[idx].name);
    return false;
  }

  Serial.printf("Writing to file %s...  [", files[idx].name);
  file.printf("%s", files[idx].data);
  file.close();
  Serial.println("done.]\n");
  Serial.flush();
}


static bool check_file(unsigned long idx) {
  if (idx < 0 || idx >= num_files) {
    Serial.println("Invalid file index.");
    return false;
  }

  File32 file = fatfs.open(files[idx].name, O_RDONLY);
  if (!file) {
    Serial.printf("Error opening file %s for reading.\n", files[idx].name);
    return false;
  }

  Serial.printf("Checking file %s... [ ", files[idx].name);
  String content = file.readString();
  file.close();

  if (strcmp(content.c_str(), files[idx].data) == 0) {
    Serial.println(" OK ]");
    return true;
  } else {
    Serial.println(" FAILED ]");
    return false;
  }

  Serial.flush();
}


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


static void check_files() {
  Serial.println("Checking files...");

  for (unsigned i = 0; i < num_files; i++) {
    if (!check_file(i)) {
      Serial.printf("File %s check failed.\n", files[i].name);
      return;
    }

    Serial.printf("File %s check passed.\n", files[i].name);
  }

  Serial.println("All files checked successfully.");
  return;
}


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    delay(
        10); // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Initializing Filesystem on external flash...");

  // Init external flash
  flash.begin();

  // Open file system on the flash
  if (!fatfs.begin(&flash)) {
    Serial.println("Error: filesystem is not existed. Please try SdFat_format "
                   "example to make one.");
    while (1) {
      yield();
      delay(1);
    }
  }

  Serial.println("initialization done.");

  write_files_to_flash();
  check_files();
}

void loop() {

}