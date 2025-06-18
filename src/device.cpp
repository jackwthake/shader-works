#include "device.h"

#include "util/helpers.h"

namespace Device {

/**
 * Initialize critical components of the device.
 */
Adafruit_ST7735 *tft = new Adafruit_ST7735(&SPI1, TFT_CS, TFT_DC, TFT_RST);

uint16_t *back_buffer = new uint16_t[screen_buffer_len];
uint16_t *front_buffer = new uint16_t[screen_buffer_len];
float *depth_buffer = new float[screen_buffer_len];

Adafruit_FlashTransport_QSPI flash_transport;
Adafruit_SPIFlash flash(&flash_transport);
FatVolume file_sys = FatFileSystem();

unsigned long now = 0, last_tick = 0;
float delta_time = 0;
bool log_debug_to_screen = true;



/**
 * Initialize the TFT display
 * Sets up the backlight, initializes the display, and sets the rotation.
 */
void tft_init() {
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH);

  if (!tft) {
    log_panic("Failed to init ST7735 driver.\n");
  }

  log("Initialized ST7735 driver.\n");

  tft->initR(INITR_BLACKTAB);
  tft->setRotation(1);

  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  tft->fillScreen(0x00);
  tft->setCursor(0, 0);

  if (!front_buffer || !back_buffer) {
    log_panic("Failed to alloc framebuffer memory.\n");
  }

  log("framebuffer Alloc successful\n");

  if (!depth_buffer) {
    log_panic("Failed to alloc depth buffer memory.\n");
  }

  log("Depth Buffer Alloc successful.\n");
  log("Screen Initialized.\n");
}



/**
 * Initialize onboard QSPI flash chip, mount filesystem, load model
 */
void spi_flash_init() {
  if (Device::flash.begin()) {
      log("Filesystem found\n");
      log("Flash chip JEDEC ID: %#04x\n", Device::flash.getJEDECID());
      log("Flash chip size: %lu bytes\n", Device::flash.size());

      // First call begin to mount the filesystem.  Check that it returns true
      // to make sure the filesystem was mounted.
      if (!Device::file_sys.begin(&Device::flash)) {
        log_panic("Failed to mount Filesystem.\n");
      }

      log("Filesystem mounted successfully.\n");
    }
}

} // namespace Device