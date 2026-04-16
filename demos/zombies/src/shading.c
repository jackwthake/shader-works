#include <math.h>

#include <SDL3/SDL.h>

#include <shader-works/renderer.h>
#include <shader-works/maths.h>
#include <shader-works/shaders.h>

#include "common/noise.h"

u32 rgb_to_u32(u8 r, u8 g, u8 b) {
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);
  return SDL_MapRGBA(format, NULL, r, g, b, 255);
}

void u32_to_rgb(u32 color, u8 *r, u8 *g, u8 *b) {
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);
  SDL_GetRGB(color, format, NULL, r, g, b);
}

// Map a value from one range to another
inline float map_range(float value, float old_min, float old_max, float new_min, float new_max) {
  return new_min + (value - old_min) * (new_max - new_min) / (old_max - old_min);
}

inline float fast_hash(int x, int y, int seed) {
  uint32_t h = seed ^ (x * 2654435761u) ^ (y * 1013904223u);
  h ^= h >> 16;
  h *= 0x85ebca6bu;
  h ^= h >> 13;
  h *= 0xc2b2ae35u;
  h ^= h >> 16;
  // Map to 0.0 - 1.0 range
  return (float)(h & 0xFFFFFF) / 16777216.0f;
}

u32 ground_frag_func(u32 input, fragment_context_t *ctx, void *args, usize argc) {
  UNUSED(input); UNUSED(args); UNUSED(argc);

  const float inv_check_size = 4.0f;
  const float speed = 1.5f;
  float time_offset = ctx->time * speed;

  float x_coord = ctx->world_pos.x;
  float z_coord = ctx->world_pos.z;

  // Axis selection (Triplanar logic)
  if (ctx->normal.y < 0.5f && ctx->normal.y > -0.5f) {
    z_coord = ctx->world_pos.y;
    if (ctx->normal.x > 0.5f || ctx->normal.x < -0.5f) {
        x_coord = ctx->world_pos.z;
    }
  }

  // Calculate grid indices
  int xi = (int)((x_coord + time_offset) * inv_check_size);
  int yi = (int)((z_coord + time_offset) * inv_check_size);

  // Fast Hash (Single call instead of noise2D)
  float noise_val = fast_hash(xi, yi, 69);
  float intensity = 0.85f + (noise_val * 0.15f);

  u8 r = (u8)(90.0f * intensity);
  u8 g = 0;
  u8 b = (u8)(160.0f * intensity);

  u32 base_color = rgb_to_u32(r, g, b);

  // Lighting and Dither
  u32 lit = default_lighting_frag_shader.func(base_color, ctx, default_lighting_frag_shader.argv, default_lighting_frag_shader.argc);
  return apply_dither_u32(lit, ctx->screen_pos, 8.0f);
}

fragment_shader_t ground_frag = {
  .valid = true,
  .argc = 0,
  .argv = NULL,
  .func = ground_frag_func
};
