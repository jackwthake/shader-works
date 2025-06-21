#include <Arduino.h>

#include <Adafruit_SPIFlash.h>
#include <SdFat_Adafruit_Fork.h>

#include "src/device.h"
#include "src/util/helpers.h"
#include "src/util/model.h"

#include "src/resource_manager.h"

Adafruit_FlashTransport_QSPI flash_transport;
Adafruit_SPIFlash flash;
FatVolume file_sys;

void setup() {
  Serial.begin(115200);
  while(!Serial) {
    // Wait for serial to be ready
  }

  // Initialise hardware
  Device::pin_init();
  Device::tft_init();

  flash = Adafruit_SPIFlash(&flash_transport);
  file_sys = FatVolume();

  if (flash.begin()) {
    log("Filesystem found\n");
    log("Flash chip JEDEC ID: %#04x\n", flash.getJEDECID());
    log("Flash chip size: %lu bytes\n", flash.size());

    // First call begin to mount the filesystem.  Check that it returns true
    // to make sure the filesystem was mounted.
    if (!file_sys.begin(&flash)) {
      log_panic("Failed to mount Filesystem.\n");
    }

    log("Dumping root directory\n");
    file_sys.dmpRootDir(&Serial);
    log("Filesystem mounted successfully.\n");
  }

  // File32 f = file_sys.open("cube.obj", O_READ);
  // if (!f.available()) {
  //   Serial.println("Failed to open cube.OBJ");
  //   while(1){}
  // }

  // Data_Resource cube_obj(0, f);
  // size_t sz;
  // const char *text = cube_obj.get_text(sz);
  // Serial.printf("Buffer size: %u, textlen: %u\n", sz, strlen(text));
  // f.close();

  // Serial.println(text);

  File32 f = file_sys.open("atlas.bmp", O_READ);
  if (!f.available()) {
    Serial.println("Failed to open cube.OBJ");
    while(1){}
  }

  Serial.println("atlas.bmp opened");
  Bitmap_Resource atlas(1, f);
  Serial.println("atlas initialized");
  Device::tft->drawRGBBitmap(0, 0, atlas.pixels, atlas.width, atlas.height);
  Serial.println("draw call finished");

  Serial.println("Finished resource test");
  Serial.flush();
  while (1) { delay(50); }
}


void loop() {}