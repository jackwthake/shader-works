#pragma once

#include <Arduino.h>
#include <SPI.h>

class Display {
public:
  // Screen dimensions
  static constexpr uint16_t width = 160;
  static constexpr uint16_t height = 128;

  Display();
  void begin();

  /**
   * @brief Draws a full-screen buffer using blocking SPI transfers.
   * @param buffer Pointer to a 160x128 uint16_t buffer.
   */
  void draw(uint16_t *buffer);

private:
  // Hardcoded pin definitions
  static constexpr int8_t cs_pin = 44;
  static constexpr int8_t dc_pin = 45;
  static constexpr int8_t rst_pin = 46;
  static constexpr int8_t backlight_pin = 47;
  
  void swap_bytes(uint16_t *src, uint32_t len);

  SPIClass *spi;
};