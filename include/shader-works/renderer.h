#ifndef SHADER_WORKS_RENDERER_H
#define SHADER_WORKS_RENDERER_H

#include <stdbool.h>
#include <shader-works/maths.h>
#include <shader-works/shaders.h>
#include <shader-works/primitives.h>

// Rendering constants
#define fov_over_2 (1.0472f / 2.0f) // 60 degrees in radians / 2

#include <shader-works/primitives.h> // model_t

// Triangle rendering context
typedef struct triangle_context_t {
  struct renderer_t *state;
  model_t *model;
  transform_t *cam;
  light_t *lights;
  usize light_count;

  vertex_shader_t *vertex_shader;
  fragment_shader_t *frag_shader;
  vertex_context_t vertex_ctx;
  fragment_context_t frag_ctx;

  int tri;

  f32 frustum_bound;
  f32 max_depth;
} triangle_context_t;

// Renderer state structure
typedef struct renderer_t {
  u32 *framebuffer;     // framebuffer, client allocated
  f32 *depthbuffer;     // depth buffer, client allocated
  u32 *texture_atlas;   // pointer to texture atlas data (static data)

  f32 time;             // Time since renderer initialization
  u64 start_time;       // Start time in milliseconds
  f32 max_depth;        // Maximum depth value for depth buffering

  u32 wireframe_mode;   // If true, render in wireframe mode, packed as u32 for alignment

  float3 cam_right, cam_up, cam_forward;
  float2 screen_dim, atlas_dim;
  f32 screen_height_world, projection_scale, frustum_bound;
} renderer_t;

// User-defined color conversion functions (must be implemented by client)
// These allow the renderer to be pixel-format agnostic
extern u32 rgb_to_u32(u8 r, u8 g, u8 b);
extern void u32_to_rgb(u32 color, u8 *r, u8 *g, u8 *b);

// Core renderer functions

// Initialize the renderer state
// framebuffer: pointer to pre-allocated framebuffer (u32 per pixel)
// depthbuffer: pointer to pre-allocated depth buffer (f32 per pixel)
// width, height: dimensions of the framebuffer
// atlas_width, atlas_height: dimensions of the texture atlas
// max_depth: maximum depth value for depth buffering
void init_renderer(renderer_t *state, u32 win_width, u32 win_height, u32 atlas_width, u32 atlas_height, u32 *framebuffer, f32 *depthbuffer, f32 max_depth);

// Update camera basis vectors based on transform
// cam: pointer to camera transform
void update_camera(renderer_t *state, transform_t *cam);

// Render a model with given camera and lights
// state: pointer to renderer state
// cam: pointer to camera transform
// model: pointer to model to render
// lights: array of lights affecting the model
// light_count: number of lights in the array
// Returns the number of triangles rendered
usize render_model(renderer_t *state, transform_t *cam, model_t *model, light_t *lights, usize light_count);

// Built-in effects

// TODO: Implement post processing shaders and convert this to new format
// Apply fog effect to the entire screen based on depth values
// fog_start: distance at which fog starts
// fog_end: distance at which fog fully obscures
// fog_r, fog_g, fog_b: RGB color of the fog
void apply_fog_to_screen(renderer_t *state, f32 fog_start, f32 fog_end, u8 fog_r, u8 fog_g, u8 fog_b);

void transform_get_basis_vectors(transform_t *t, float3 *ihat, float3 *jhat, float3 *khat);
void transform_get_inverse_basis_vectors(transform_t *t, float3 *ihat, float3 *jhat, float3 *khat);

#endif // SHADER_WORKS_RENDERER_H
