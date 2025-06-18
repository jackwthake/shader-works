#include "helpers.h"

#include <Arduino.h>
#include <stdarg.h>

#include <Adafruit_GFX.h>

#include "../device.h"

void _swap_int(int& a, int& b) {
    int tmp = a;
    a = b;
    b = tmp;
}


void _swap_ptr(void *a, void *b) {
    void *tmp = a;
    a = b;
    b = tmp;
}


/**
 * packs 3 8 bit RGB values into a 16 bit BGR565 value.
 * The 16 bit value is packed as follows:
 *  - 5 bits for blue (B)
 *  - 6 bits for green (G)
 *  - 5 bits for red (R) 
 */
uint16_t rgb_to_565(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t BGRColor = b >> 3;
    BGRColor         |= (g & 0xFC) << 3;
    BGRColor         |= (r & 0xF8) << 8;

    return BGRColor;
}


/**
 * printf function that logs messages to the serial console.
 * It prefixes each message with the current time in milliseconds.
 */
void log(const char *format, ...) {
    va_list args;
    va_start(args, format);
    Serial.printf("[ %u ]: ", millis());
    Serial.printf(format, args);
    if (Device::log_debug_to_screen)
        Device::tft->printf(format, args);
    va_end(args);
}


/**
 * Logs a panic message to the serial console and enters an infinite loop.
 */
void log_panic(const char *format, ...) {
    va_list args;
    va_start(args, format);
    Serial.printf("[ %u ]: ", millis());
    Serial.printf(format, args);
    if (Device::log_debug_to_screen)
        Device::tft->printf(format, args);
    va_end(args);
    
    Serial.println();
    Serial.flush();
    
    while (true) {
        delay(1000); // Infinite loop to halt execution
    }
}