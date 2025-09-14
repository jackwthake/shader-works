#ifndef __DEF_H__
#define __DEF_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define WIN_WIDTH 480
#define WIN_HEIGHT 240
#define WIN_SCALE 4
#define WIN_TITLE "CPU Renderer"

typedef int8_t i8;           // Signed 8-bit integer
typedef uint8_t u8;          // Unsigned 8-bit integer
typedef int16_t i16;         // Signed 16-bit integer
typedef uint16_t u16;        // Unsigned 16-bit integer
typedef int32_t i32;         // Signed 32-bit integer
typedef uint32_t u32;        // Unsigned 32-bit integer
typedef int64_t i64;         // Signed 64-bit integer
typedef uint64_t u64;        // Unsigned 64-bit integer

typedef float f32;           // 32-bit floating point
typedef double f64;          // 64-bit floating point

typedef size_t usize;        // Unsigned integer type for sizes
typedef unsigned char uchar; // Unsigned char type


struct game_state_t {
  bool running;

  SDL_Window *window;
  SDL_Renderer *renderer;
  u32 framebuffer[WIN_WIDTH * WIN_HEIGHT];
};

#endif /* __DEF_H__ */