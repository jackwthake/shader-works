#include "display.h"

// ST7735 Commands
#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT  0x11
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_MADCTL  0x36
#define ST7735_COLMOD  0x3A

Display::Display() {
  spi = &SPI1;
}

void Display::begin() {
  // Pin Initialization
  pinMode(cs_pin, OUTPUT);
  digitalWrite(cs_pin, HIGH);
  pinMode(dc_pin, OUTPUT);
  digitalWrite(dc_pin, HIGH);
  pinMode(backlight_pin, OUTPUT);
  digitalWrite(backlight_pin, HIGH);

  // Hardware Reset
  if (rst_pin >= 0) {
    digitalWrite(rst_pin, HIGH); delay(5);
    digitalWrite(rst_pin, LOW);  delay(20);
    digitalWrite(rst_pin, HIGH); delay(150);
  }
  
  spi->begin();
  SPISettings settings(24000000, MSBFIRST, SPI_MODE0);
  
  // Minimal Init Sequence (known good from our test)
  spi->beginTransaction(settings);
  digitalWrite(cs_pin, LOW);
  digitalWrite(dc_pin, LOW); spi->transfer(ST7735_SWRESET);
  digitalWrite(cs_pin, HIGH);
  spi->endTransaction();
  delay(150);

  spi->beginTransaction(settings);
  digitalWrite(cs_pin, LOW);
  digitalWrite(dc_pin, LOW); spi->transfer(ST7735_SLPOUT);
  digitalWrite(cs_pin, HIGH);
  spi->endTransaction();
  delay(250);

  spi->beginTransaction(settings);
  digitalWrite(cs_pin, LOW);
  digitalWrite(dc_pin, LOW); spi->transfer(ST7735_MADCTL);
  digitalWrite(dc_pin, HIGH); spi->transfer(0x60);
  
  digitalWrite(dc_pin, LOW); spi->transfer(ST7735_COLMOD);
  digitalWrite(dc_pin, HIGH); spi->transfer(0x05);
  digitalWrite(cs_pin, HIGH);
  spi->endTransaction();
  
  spi->beginTransaction(settings);
  digitalWrite(cs_pin, LOW);
  digitalWrite(dc_pin, LOW); spi->transfer(ST7735_DISPON);
  digitalWrite(cs_pin, HIGH);
  spi->endTransaction();
}

void Display::draw(uint16_t *buffer) {
  uint32_t n = width * height;
  for (uint32_t i = 0; i < n / 2; ++i) {
    uint16_t temp = buffer[i];
    buffer[i] = buffer[n - 1 - i];
    buffer[n - 1 - i] = temp;
  }


  SPISettings settings(24000000, MSBFIRST, SPI_MODE0);
  spi->beginTransaction(settings);
  digitalWrite(cs_pin, LOW);
  
  // Set Address Window to Full Screen
  digitalWrite(dc_pin, LOW); spi->transfer(ST7735_CASET);
  digitalWrite(dc_pin, HIGH);
  uint8_t caset[] = {0, 0, (uint8_t)((width - 1) >> 8), (uint8_t)((width - 1) & 0xFF)};
  spi->transfer(caset, 4);

  digitalWrite(dc_pin, LOW); spi->transfer(ST7735_RASET);
  digitalWrite(dc_pin, HIGH);
  uint8_t raset[] = {0, 0, (uint8_t)((height - 1) >> 8), (uint8_t)((height - 1) & 0xFF)};
  spi->transfer(raset, 4);
  
  // Send Write to RAM command
  digitalWrite(dc_pin, LOW);
  spi->transfer(ST7735_RAMWR);
  
  // Prepare buffer and switch to data mode
  swap_bytes(buffer, width * height);
  digitalWrite(dc_pin, HIGH);

  // Send buffer data using blocking SPI transfers
  spi->transfer(buffer, width * height * 2);
  
  digitalWrite(cs_pin, HIGH);
  spi->endTransaction();
}

void Display::swap_bytes(uint16_t *src, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    src[i] = __builtin_bswap16(src[i]);
  }
}