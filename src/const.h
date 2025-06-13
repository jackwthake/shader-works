#pragma once

#ifndef CONST_H
#define CONST_H

#define USE_SPI_DMA
#include <SPI.h>
#include <Adafruit_GFX.h>    // Not used, but required for ST7735
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735

#include <Adafruit_SPIFlash.h> // SPI Flash library for QSPI flash memory

namespace Device  {
  constexpr int width = 160, height = 128;
  constexpr int screen_buffer_len = width * height;
  constexpr float max_depth = 100.0f; // Maximum depth value for the depth buffer
  constexpr unsigned long tick_interval = 50; // 20 ticks per second (1000ms / 20 = 50ms)
  
  
  extern Adafruit_ST7735 *tft;

  extern Adafruit_FlashTransport_QSPI *flash_transport;
  extern Adafruit_SPIFlash *flash;
  extern FatFileSystem file_sys;

  extern uint16_t *front_buffer;
  extern uint16_t *back_buffer;
  extern float *depth_buffer;

  enum pins {
    TFT_CS = 44,            // Chip select for the TFT display
    TFT_DC = 45,            // Data/Command select for the TFT display
    TFT_RST = 46,           // Reset pin for the TFT display
    TFT_BACKLIGHT = 47      // Backlight control pin for the TFT display
  };
};


#endif // CONST_H