#include <math.h>
#include <stdlib.h>

#include <SDL3/SDL.h>

#include <shader-works/renderer.h>
#include <shader-works/maths.h>
#include <shader-works/shaders.h>

#include "common/noise.h"

#include "world.h"

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

const int num_particles = 50, particle_radius = 32;
static float3 particles[50] = { 0 };

float3 get_particle_pos(world_t *world, transform_t *cam) {
  float x, y = 0.0f, z, sector_floor = 0.0f;
  bool valid = false;

  while (!valid) {
    // generate in a radius around the player camera
    float angle = (rand() / (float)RAND_MAX) * 2.0f * PI;
    float radius = (rand() / (float)RAND_MAX) * particle_radius + 1.0f; // 1 to 10 units away
    x = cam->position.x + cosf(angle) * radius;
    z = cam->position.z + sinf(angle) * radius;

    if (point_in_sector(world, (float3){x, y, z}, &sector_floor))
      valid = true;
  }

  y = sector_floor + (rand() % 100) / 100.0f * 2.0f; // Random height above floor (0 to 2)

  return (float3){
    x, y, z
  };
}

void init_particles(world_t *world, transform_t *cam) {
  for (int i = 0; i < num_particles; i++) {
    particles[i] = get_particle_pos(world, cam);
  }
}

void apply_dust_particles(renderer_t *state, world_t *world, transform_t *cam) {
  for (int i = 0; i < num_particles; i++) {
    float3 *p = &particles[i];
    p->y += 0.01f; // Move up
    p->x += (fast_hash((int)(p->x * 10), (int)(p->y * 10), 123) - 0.5f) * 0.02f; // Small random horizontal movement
    p->z += (fast_hash((int)(p->z * 10), (int)(p->y * 10), 321) - 0.5f) * 0.02f;

    // If particle goes above a certain height, reset it to the bottom
    float floor_height;
    point_in_sector(world, (float3){p->x, p->y, p->z}, &floor_height);

    if (p->y > floor_height + 2.0f) {
      *p = get_particle_pos(world, cam);
    }

    // regenerate if too far from camera to avoid rendering overhead, simple distance check
    float dx = p->x - cam->position.x;
    float dy = p->y - cam->position.y;
    float dz = p->z - cam->position.z;
    float dist_sq = dx*dx + dy*dy + dz*dz;
    if (dist_sq > particle_radius * particle_radius) { // If more than 10 units away, reset
      *p = get_particle_pos(world, cam);
    }

    // pass particle color through lighting shader to blend in with world
    u32 out = default_lighting_frag_shader.func(rgb_to_u32(200, 200, 200), &(fragment_context_t){
      .world_pos = *p,
      .normal = (float3){0, 1, 0}, // Upward facing normal for lighting
      .view_dir = float3_normalize(float3_sub(cam->position, *p)),
      .time = state->time,
      .light = world->lights,
      .light_count = world->num_lights
    }, default_lighting_frag_shader.argv, default_lighting_frag_shader.argc);

    render_point(state, cam, *p, out);
  }
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
