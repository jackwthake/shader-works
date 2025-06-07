#include "helpers.h"

#include <Arduino.h>
#include <stdarg.h>


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


uint16_t rgb_to_565(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t BGRColor = b >> 3;
    BGRColor         |= (g & 0xFC) << 3;
    BGRColor         |= (r & 0xF8) << 8;

    return BGRColor;
}


void log(const char *format, ...) {
    va_list args;
    va_start(args, format);
    Serial.printf("%u: ", millis());
    Serial.printf(format, args);
    Serial.flush();
    va_end(args);
}


void log_panic(const char *format, ...) {
    va_list args;
    va_start(args, format);
    Serial.printf("%u: ", millis());
    Serial.printf(format, args);
    va_end(args);
    
    Serial.println();
    Serial.flush();
    
    while (true) {
        delay(1000); // Infinite loop to halt execution
    }
}