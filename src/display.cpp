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

/**
 * Reverses the elements of a uint16_t array in-place.
 * @param buffer A pointer to the uint16_t array to be reversed.
 * @param n The number of elements in the array.
 */
static void reverse_buffer(uint16_t* buffer, size_t n) {
  // Guard against null pointers or empty arrays.
  if (!buffer || n == 0) {
    return;
  }

  // Initialize two pointers: one at the start and one at the end.
  size_t start = 0;
  size_t end = n - 1;

  // Loop until the pointers meet or cross in the middle.
  while (start < end) {
    // Swap the elements at the start and end positions.
    uint16_t temp = buffer[start];
    buffer[start] = buffer[end];
    buffer[end] = temp;

    // Move the pointers towards the center.
    start++;
    end--;
  }
}


Display::Display() {
  spi = &SPI1;
}


/**
 * Initializes the display by setting up the pins and sending the necessary commands.
 * This function must be called before any drawing operations.
 */
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


/**
 * Draws a full-screen buffer using blocking SPI transfers.
 * @param buffer Pointer to a 160x128 uint16_t buffer.
 */
void Display::draw(uint16_t *buffer) {
  reverse_buffer(buffer, width * height); // Reverse the buffer for correct display order


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


/**
 * Swaps the bytes of each 16-bit value in the array.
 * This is useful for converting between little-endian and big-endian formats.
 * @param src Pointer to the source array of uint16_t values.
 * @param len The number of elements in the array.
 */
void Display::swap_bytes(uint16_t *src, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    src[i] = __builtin_bswap16(src[i]);
  }
}