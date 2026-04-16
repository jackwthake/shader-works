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

u32 ground_frag_func(u32 input, fragment_context_t *ctx, void *args, usize argc) {
  UNUSED(input); UNUSED(args); UNUSED(argc);

  float check_size = 0.25f;
  float speed = 1.5f;

  float x_coord = ctx->world_pos.x;
  float y_coord = ctx->world_pos.z;

  // If the absolute value of the Y normal is low, we are looking at a wall
  if (fabsf(ctx->normal.y) < 0.5f) {
    y_coord = ctx->world_pos.y;

    if (fabsf(ctx->normal.x) > 0.5f) {
        x_coord = ctx->world_pos.z;
    }
  }

  float x = floorf((x_coord + ctx->time * speed) / check_size);
  float y = floorf((y_coord + ctx->time * speed) / check_size);

  float intensity = map_range(noise2D(x, y, 69), -1.0f, 1.0f, 0.85f, 1.0f);

  u8 r = (u8)(90.f * intensity);
  u8 g = (u8)(0.f * intensity);
  u8 b = (u8)(160.f * intensity);

  u32 lit = default_lighting_frag_shader.func(rgb_to_u32(r, g, b), ctx, default_lighting_frag_shader.argv, default_lighting_frag_shader.argc);
  return apply_dither_u32(lit, ctx->screen_pos, 8.0f);
}

fragment_shader_t ground_frag = {
  .valid = true,
  .argc = 0,
  .argv = NULL,
  .func = ground_frag_func
};
