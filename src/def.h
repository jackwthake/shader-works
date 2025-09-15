#ifndef __DEF_H__
#define __DEF_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <SDL3/SDL.h>

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

typedef struct {
  f32 x, y;
} float2;

typedef struct {
  f32 x, y, z;
} float3;

typedef struct {
  f32 yaw;
  f32 pitch;
  float3 position;
} transform_t;

typedef struct {
  float3 *vertices; // List of vertices
  float2 *uvs; // pointer to texture coordinates (static data)

  usize num_vertices;
  usize num_uvs;
  
  float3 scale;
  transform_t transform;
} model_t;

typedef struct {
  bool running;

  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture* framebuffer_tex;
  u32 framebuffer[WIN_WIDTH * WIN_HEIGHT];
  f32 depthbuffer[WIN_WIDTH * WIN_HEIGHT];

  float3 cam_right, cam_up, cam_forward;
  float2 screen_dim;
  f32 screen_height_world, projection_scale, frustum_bound;

  bool use_textures;
  u16 *texture_atlas; // pointer to texture atlas data (static data)
} game_state_t;

#endif /* __DEF_H__ */