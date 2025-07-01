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
#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5
#define ST7735_INVOFF  0x20
#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1
#define ST7735_NORON   0x13


uint8_t init_cmds[] = {           // 7735R init
  19,                             // 19 commands in list:
  ST7735_SWRESET,   0x80,         //  1: Software reset, 0 args, w/delay
    150,                          //     150 ms delay
  ST7735_SLPOUT,    0x80,         //  2: Out of sleep mode, 0 args, w/delay
    255,                          //     255 ms delay
  ST7735_FRMCTR1, 3,              //  3: Framerate ctrl - normal mode, 3 args:
    0x01, 0x2C, 0x2D,             //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
  ST7735_FRMCTR2, 3,              //  4: Framerate ctrl - idle mode, 3 args:
    0x01, 0x2C, 0x2D,             //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
  ST7735_FRMCTR3, 6,              //  5: Framerate ctrl - partial mode, 6 args:
    0x01, 0x2C, 0x2D,             //     Dot inversion mode
    0x01, 0x2C, 0x2D,             //     Line inversion mode
  ST7735_INVCTR,  1,              //  6: Display inversion ctrl, 1 arg:
    0x07,                         //     No inversion
  ST7735_PWCTR1,  3,              //  7: Power control, 3 args:
    0xA2,
    0x02,                         //     -4.6V
    0x84,                         //     AUTO mode
  ST7735_PWCTR2,  1,              //  8: Power control, 1 arg:
    0xC5,                         //     VGH25=2.4C VGSEL=-10 VGH=3 * AVDD
  ST7735_PWCTR3,  2,              //  9: Power control, 2 args:
    0x0A,                         //     Opamp current small
    0x00,                         //     Boost frequency
  ST7735_PWCTR4,  2,              // 10: Power control, 2 args:
    0x8A,                         //     BCLK/2,
    0x2A,                         //     opamp current small & medium low
  ST7735_PWCTR5,  2,              // 11: Power control, 2 args:
    0x8A, 0xEE,
  ST7735_VMCTR1,  1,              // 12: Power control, 1 arg:
    0x0E,
  ST7735_INVOFF,  0,              // 13: Don't invert display, no args
  ST7735_MADCTL,  1,              // 14: Memory access control, 1 arg:
    0xA0,                         //     Row addr order, row/col exchange, col addr order
  ST7735_COLMOD,  1,              // 15: Set color mode, 1 arg:
    0x05,                         //     16-bit color  
  ST7735_GMCTRP1, 16,             // 16: Gamma adjustments (pos. polarity), 16 args:
    0x02, 0x1c, 0x07, 0x12,       //     (Not entirely necessary, but provides
    0x37, 0x32, 0x29, 0x2d,       //      accurate colors)
    0x29, 0x25, 0x2B, 0x39,
    0x00, 0x01, 0x03, 0x10,
  ST7735_GMCTRN1, 16,             //  17: Gamma Adjustments (neg. polarity), 16 args + delay:
    0x03, 0x1d, 0x07, 0x06,       //     (Not entirely necessary, but provides
    0x2E, 0x2C, 0x29, 0x2D,       //      accurate colors)
    0x2E, 0x2E, 0x37, 0x3F,
    0x00, 0x00, 0x02, 0x10,
  ST7735_NORON,   0x80,           //  18: Normal display on, no args, w/delay
    10,                           //     10 ms delay
  ST7735_DISPON,   0x80,          //  19: Main screen turn on, no args w/delay
    100                           //     100 ms delay
};



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

  uint8_t cmd_count = init_cmds[0];
  uint8_t *cmd = init_cmds + 1;
  for (uint8_t i = 0; i < cmd_count; i++) {
    uint8_t command = *cmd++;
    uint8_t arg_count = *cmd++;
    
    if (arg_count & 0x80) {
      // Command with delay
      arg_count &= 0x7F; // Remove delay flag
      if (arg_count == 0) {
        // No arguments, just send the command
        send_command(command);
      } else {
        // Send command with arguments
        send_command(command, cmd, arg_count);
        cmd += arg_count; // Move past arguments
      }
      
      // Handle delay
      uint8_t delay_time = *cmd++;
      delay(delay_time);
    } else {
      // Command without delay
      if (arg_count == 0) {
        // No arguments, just send the command
        send_command(command);
      } else {
        // Send command with arguments
        send_command(command, cmd, arg_count);
        cmd += arg_count; // Move past arguments
      }
    }
  }

  digitalWrite(cs_pin, HIGH);
  spi->endTransaction();
}


/**
 * Draws a full-screen buffer using blocking SPI transfers.
 * @param buffer Pointer to a 160x128 uint16_t buffer.
 */
void Display::draw(uint16_t *buffer) {
  SPISettings settings(24000000, MSBFIRST, SPI_MODE0);
  spi->beginTransaction(settings);
  digitalWrite(cs_pin, LOW);
  
  // Set Address Window to Full Screen
  uint8_t caset[] = {0, 0, (uint8_t)((width - 1) >> 8), (uint8_t)((width - 1) & 0xFF)};
  send_command(ST7735_CASET, caset, 4);
  
  uint8_t raset[] = {0, 0, (uint8_t)((height - 1) >> 8), (uint8_t)((height - 1) & 0xFF)};
  send_command(ST7735_RASET, raset, 4);

  // Send Write to RAM command
  send_command(ST7735_RAMWR);
  
  // Prepare buffer and switch to data mode
  swap_bytes(buffer, width * height);
  digitalWrite(dc_pin, HIGH);

  // Send buffer data using blocking SPI transfers
  spi->transfer((uint8_t*)buffer, width * height * 2);
  
  digitalWrite(cs_pin, HIGH);
  spi->endTransaction();
}


/**
 * Sends a command to the display.
 * @param data The command byte to send.
 */
void Display::send_command(uint8_t data) {
  digitalWrite(dc_pin, LOW);
  spi->transfer(data);
}


/**
 * Sends a command with arguments to the display.
 * @param cmd The command byte to send.
 * @param data Pointer to the data array to send.
 * @param len The length of the data array.
 */
void Display::send_command(uint8_t cmd, uint8_t *data, size_t len) {
  digitalWrite(dc_pin, LOW);
  spi->transfer(cmd);
  digitalWrite(dc_pin, HIGH);
  spi->transfer(data, len);
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