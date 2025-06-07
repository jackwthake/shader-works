#ifndef HELPERS_H
#define HELPERS_H

#include <stdint.h>

void _swap_ptr(void *a, void *b);
void _swap_int(int& a, int& b);

uint16_t rgb_to_565(uint8_t r, uint8_t g, uint8_t b);

void log(const char *format, ...);
void log_panic(const char *format, ...);

#endif // HELPERS_H