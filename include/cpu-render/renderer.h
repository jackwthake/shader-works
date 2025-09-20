#ifndef CPU_RENDER_RENDERER_H
#define CPU_RENDER_RENDERER_H

#include <stdbool.h>
#include <SDL3/SDL.h>
#include "maths.h"
#include "shaders.h"
#include "primitives.h"

// Rendering constants
#define fov_over_2 (1.0472f / 2.0f) // 60 degrees in radians / 2

// Renderer state structure
typedef struct {
  u32 *framebuffer;
  f32 *depthbuffer;

  float3 cam_right, cam_up, cam_forward;
  float2 screen_dim, atlas_dim;
  f32 screen_height_world, projection_scale, frustum_bound;
  f32 max_depth;

  bool wireframe_mode; // If true, render in wireframe mode

  u32 *texture_atlas; // pointer to texture atlas data (static data)
} renderer_t;

// Utility functions for color conversion
u32 rgb_to_u32(u8 r, u8 g, u8 b);
void u32_to_rgb(u32 color, u8 *r, u8 *g, u8 *b);

// Core renderer functions
void init_renderer(renderer_t *state, u32 win_width, u32 win_height, u32 atlas_width, u32 atlas_height, u32 *framebuffer, f32 *depthbuffer, f32 max_depth);
void update_camera(renderer_t *state, transform_t *cam);
usize render_model(renderer_t *state, transform_t *cam, model_t *model);

// Built-in effects
void apply_fog_to_screen(renderer_t *state, f32 fog_start, f32 fog_end, u8 fog_r, u8 fog_g, u8 fog_b);

#endif // CPU_RENDER_RENDERER_H