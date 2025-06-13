#include <Arduino.h>

#include "src/const.h"
#include "src/util/helpers.h"
#include "src/renderer.h"
#include "src/util/model.h"

unsigned long now, last_tick = 0, tick_count = 0, frame_count = 0, last_report = 0;
float delta_time = 0.0;
bool first_frame = true;


Adafruit_ST7735 *Device::tft = new Adafruit_ST7735(&SPI1, Device::TFT_CS, Device::TFT_DC, Device::TFT_RST);

uint16_t *Device::back_buffer = new uint16_t[Device::screen_buffer_len];
uint16_t *Device::front_buffer = new uint16_t[Device::screen_buffer_len];
float *Device::depth_buffer = new float[Device::screen_buffer_len];

Adafruit_FlashTransport_QSPI *Device::flash_transport = new Adafruit_FlashTransport_QSPI();
Adafruit_SPIFlash *Device::flash = new Adafruit_SPIFlash(Device::flash_transport);
FatFileSystem Device::file_sys = FatFileSystem();


static std::string cube_obj = "v 1.000000 -1.000000 -1.000000\n"
                      "v 1.000000 -1.000000 1.000000\n"
                      "v -1.000000 -1.000000 1.000000\n"
                      "v -1.000000 -1.000000 -1.000000\n"
                      "v 1.000000 1.000000 -0.999999\n"
                      "v 0.999999 1.000000 1.000001\n"
                      "v -1.000000 1.000000 1.000000\n"
                      "v -1.000000 1.000000 -1.000000\n"
                      "f 2/1/1 3/2/1 4/3/1\n"
                      "f 8/1/2 7/4/2 6/5/2\n"
                      "f 5/6/3 6/7/3 2/8/3\n"
                      "f 6/8/4 7/5/4 3/4/4\n"
                      "f 3/9/5 7/10/5 8/11/5\n"
                      "f 1/12/6 4/13/6 8/11/6\n"
                      "f 1/4/1 2/1/1 4/3/1\n"
                      "f 5/14/2 8/1/2 6/5/2\n"
                      "f 1/12/3 5/6/3 2/8/3\n"
                      "f 2/12/4 6/8/4 3/4/4\n"
                      "f 4/13/5 3/9/5 8/11/5\n"
                      "f 5/6/6 1/12/6 8/11/6\n";

Model cube, cube2;


/**
 * Initialize the TFT display
 * Sets up the backlight, initializes the display, and sets the rotation.
 */
void tft_init(void) {
  pinMode(Device::TFT_BACKLIGHT, OUTPUT);
  digitalWrite(Device::TFT_BACKLIGHT, HIGH);

  if (!Device::tft) {
    log_panic("Failed to initialize ST7735 display driver.\n");
  }

  log("Initialized ST7735 display driver.\n");

  Device::tft->initR(INITR_BLACKTAB);
  Device::tft->setRotation(1);
  Device::tft->fillScreen(0x00);

  pinMode(Device::TFT_CS, OUTPUT);
  digitalWrite(Device::TFT_CS, HIGH);

  if (!Device::front_buffer || !Device::back_buffer) {
    log_panic("Failed to allocate framebuffer memory.\n");
  }

  log("Screen Buffer Allocated successfully.\n");

  if (!Device::depth_buffer) {
    log_panic("Failed to allocate depth buffer memory.\n");
  }

  log("Depth Buffer Allocated successfully.\n");
  log("Screen Initialized.\n");
}


void spi_flash_init() {
  if (!Device::flash_transport || !Device::flash) {
    log_panic("Failed to initialize memory for QSPI flash transport or filesystem.\n");
  }

  log("Initialized memory for QSPI flash transport and filesystem.\n");

  if (Device::flash->begin()) {
      log("QSPI filesystem found\n");
      log("QSPI flash chip JEDEC ID: %#04x\n", Device::flash->getJEDECID());
      log("QSPI flash chip size: %lu bytes\n", Device::flash->size());

      // First call begin to mount the filesystem.  Check that it returns true
      // to make sure the filesystem was mounted.
      if (!Device::file_sys.begin(Device::flash)) {
        log_panic("Failed to mount QSPI filesystem.\n");
      }

      log("QSPI filesystem mounted successfully.\n");
    }
}


void setup() {
  Serial.begin(9600);
  delay(250); // Wait for serial to initialize

  tft_init();

  spi_flash_init();
  
  log("Initialized.\n");


  // Initialize the cube model
  cube.vertices = read_obj(cube_obj);
  cube2.vertices = read_obj(cube_obj);

  cube.cols = {
      random_color(), random_color(), random_color(), random_color(),
      random_color(), random_color(), random_color(), random_color(),
      random_color(), random_color(), random_color(), random_color()
  };

  cube2.cols = {
      random_color(), random_color(), random_color(), random_color(),
      random_color(), random_color(), random_color(), random_color(),
      random_color(), random_color(), random_color(), random_color()
  };

  cube.transform.yaw = 1.5f;
  cube.transform.pitch = 3.f;
  cube.transform.position = { 0, 0, 5 };

  cube2.transform.position = {-2, -2, 8};
}


void render_frame() {
  memset(Device::back_buffer, 0x00, Device::screen_buffer_len * sizeof(uint16_t));
  memset(Device::depth_buffer, Device::max_depth, Device::screen_buffer_len * sizeof(float));

  render_model(Device::back_buffer, cube);
  render_model(Device::back_buffer, cube2);

  if (!first_frame) { // finish up the previous frame
    Device::tft->dmaWait();
    Device::tft->endWrite();

    _swap_ptr(Device::front_buffer, Device::back_buffer);
  } else {
    first_frame = false;
  }

  // Blit the framebuffer to the screen
  Device::tft->setAddrWindow(0, 0, Device::width, Device::height);
  Device::tft->startWrite();
  Device::tft->writePixels(Device::back_buffer, Device::width * Device::height, false);
}


void loop() {
  now = millis();

  // Process all pending ticks to maintain a fixed timestep
  int ticks_processed = 0;
  while ((now - last_tick) >= Device::tick_interval && ticks_processed < 5) { // Limit catch-up to avoid spiral of death
    delta_time = Device::tick_interval / 1000.0f; // fixed delta time in seconds
    last_tick += Device::tick_interval;

    cube.transform.yaw += 1.f * delta_time; // Rotate cube
    cube.transform.pitch += 1.5f * delta_time; // Rotate cube

    cube2.transform.position.y = sin((millis() / 75)  * delta_time) * 5;
    cube2.transform.yaw += 2 * delta_time;

    tick_count++;
    ticks_processed++;
  }

  // Tickrate and framerate reporting
  if (now - last_report >= 1000) { // Every 1 second
    log("TPS: %lu ticks/sec\n", tick_count);
    log("FPS: %lu frames/sec\n", frame_count);
    tick_count = 0;
    frame_count = 0;
    last_report = now;
  }

  /* Update back buffer, swap buffers and blit framebuffer to screen */
  render_frame();
  ++frame_count;
}