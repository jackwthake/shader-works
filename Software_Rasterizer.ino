#include <Arduino.h>

#define USE_SPI_DMA
#include <Adafruit_GFX.h>    // Not used, but required for ST7735
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>

#include "src/util/helpers.h"

#define TFT_CS        44 // PyBadge/PyGamer display control pins: chip select
#define TFT_RST       46 // Display reset
#define TFT_DC        45 // Display data/command select
#define TFT_BACKLIGHT 47 // Display backlight pin

struct Device {
  static constexpr int width = 160, height = 128;
  static constexpr int screen_buffer_len = width * height;
  Adafruit_ST7735 *tft;

  uint16_t *front_buffer;
  uint16_t *back_buffer;
} device;

const unsigned long tick_interval = 50; // 20 ticks per second (1000ms / 20 = 50ms)

unsigned long now, last_tick = 0, tick_count = 0, frame_count = 0, last_report = 0;
float delta_time = 0.0;
bool first_frame = true;


// Draw a filled triangle into a framebuffer (1D array of uint16_t)
// framebuffer: pointer to framebuffer (size width*height)
// width, height: dimensions of framebuffer
// (x0, y0), (x1, y1), (x2, y2): triangle vertices
// color: 16-bit color value
void draw_fill_triangle(uint16_t* framebuffer,
                          int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) {
    // Sort vertices by y (y0 <= y1 <= y2)
    if (y0 > y1) { _swap_int(y0, y1); _swap_int(x0, x1); }
    if (y1 > y2) { _swap_int(y1, y2); _swap_int(x1, x2); }
    if (y0 > y1) { _swap_int(y0, y1); _swap_int(x0, x1); }

    auto edge_interp = [](int x0, int y0, int x1, int y1, int y) -> int {
        if (y1 == y0) return x0;
        return x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    };

    // Fill from y0 to y2
    for (int y = y0; y <= y2; ++y) {
        if (y < 0 || y >= device.height) continue;
        int xa, xb;
        if (y < y1) {
            xa = edge_interp(x0, y0, x1, y1, y);
            xb = edge_interp(x0, y0, x2, y2, y);
        } else {
            xa = edge_interp(x1, y1, x2, y2, y);
            xb = edge_interp(x0, y0, x2, y2, y);
        }
        if (xa > xb) _swap_int(xa, xb);
        xa = max(xa, 0);
        xb = min(xb, device.width - 1);
        for (int x = xa; x <= xb; ++x) {
            framebuffer[y * device.width + x] = color;
        }
    }
}


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

  tft_init();
  log("Initialized.\n");
}


int x = device.width / 2;
int y = device.height / 2;
void render_frame() {
  memset(device.back_buffer, 0x00, device.screen_buffer_len * sizeof(uint16_t));

  draw_fill_triangle(device.back_buffer, x, y, x + 30, y, x + 15, y + 30, rgb_to_565(255, 0, 255));

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

    x = (cos(millis() / 10 * delta_time) * 50) + (device.width / 2);
    y = (sin(millis() / 10 * delta_time) * 30) + (device.height / 2);

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
  frame_count++;
}
