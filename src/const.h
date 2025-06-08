#ifndef CONST_H
#define CONST_H

#define USE_SPI_DMA
#include <Adafruit_GFX.h>    // Not used, but required for ST7735
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>

#define TFT_CS        44 // chip select
#define TFT_RST       46 // Display reset
#define TFT_DC        45 // Display data/command select
#define TFT_BACKLIGHT 47 // Display backlight pin

struct Device {
  static constexpr int width = 160, height = 128;
  static constexpr int screen_buffer_len = width * height;
  static constexpr float max_depth = 100.0f; // Maximum depth value for the depth buffer
  Adafruit_ST7735 *tft;

  uint16_t *front_buffer;
  uint16_t *back_buffer;
  float *depth_buffer;
};

const unsigned long tick_interval = 50; // 20 ticks per second (1000ms / 20 = 50ms)

#endif // CONST_H