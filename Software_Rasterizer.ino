#include <Arduino.h>

#include "src/const.h"
#include "src/util/helpers.h"
#include "src/renderer.h"
#include "src/util/model.h"

Device device;

unsigned long now, last_tick = 0, tick_count = 0, frame_count = 0, last_report = 0;
float delta_time = 0.0;
bool first_frame = true;


Model cube;


/**
 * Initialize the TFT display
 * Sets up the backlight, initializes the display, and sets the rotation.
 */
void tft_init(void) {
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH);

  device.tft->initR(INITR_BLACKTAB);
  device.tft->setRotation(1);
  device.tft->fillScreen(0xFF);

  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  log("Screen Initialized.\n");
}


void setup() {
  Serial.begin(9600);
  delay(75);

  device.tft = new Adafruit_ST7735(&SPI1, TFT_CS, TFT_DC, TFT_RST);
  if (!device.tft) {
    log_panic("Failed to initialize ST7735 display driver.\n");
  }

  log("Initialized ST7735 display driver.\n");

  device.back_buffer = new uint16_t[device.screen_buffer_len];
  device.front_buffer = new uint16_t[device.screen_buffer_len];
  if (!device.front_buffer || !device.back_buffer) {
    log_panic("Failed to allocate framebuffer memory.\n");
  }

  device.depth_buffer = new float[device.screen_buffer_len];
  if (!device.depth_buffer) {
    log_panic("Failed to allocate depth buffer memory.\n");
  }

  tft_init();
  log("Initialized.\n");


  // Initialize the cube model
  cube.vertices = read_obj(
      "v 1.000000 -1.000000 -1.000000\n"
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
      "f 5/6/6 1/12/6 8/11/6\n");

  cube.cols = {
      random_color(), random_color(), random_color(), random_color(),
      random_color(), random_color(), random_color(), random_color(),
      random_color(), random_color(), random_color(), random_color()
  };

  cube.transform.yaw = 1.5f;
  cube.transform.pitch = 3.f;
  cube.transform.position = { 0, 0, 5 };
}


void render_frame() {
  memset(device.back_buffer, 0x00, device.screen_buffer_len * sizeof(uint16_t));
  memset(device.depth_buffer, device.max_depth, device.screen_buffer_len * sizeof(float));

  render_model(device.back_buffer, cube);

  if (!first_frame) { // finish up the previous frame
    device.tft->dmaWait();
    device.tft->endWrite();

    _swap_ptr(device.front_buffer, device.back_buffer);
  } else {
    first_frame = false;
  }

  // Blit the framebuffer to the screen
  device.tft->setAddrWindow(0, 0, device.width, device.height);
  device.tft->startWrite();
  device.tft->writePixels(device.back_buffer, device.width * device.height, false);
}


void loop() {
  now = millis();

  // Process all pending ticks to maintain a fixed timestep
  int ticks_processed = 0;
  while ((now - last_tick) >= tick_interval && ticks_processed < 5) { // Limit catch-up to avoid spiral of death
    delta_time = tick_interval / 1000.0f; // fixed delta time in seconds
    last_tick += tick_interval;

    cube.transform.yaw += 1.f * delta_time; // Rotate cube
    cube.transform.pitch += 1.5f * delta_time; // Rotate cube

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