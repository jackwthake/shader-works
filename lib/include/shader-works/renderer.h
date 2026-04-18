#ifndef SHADER_WORKS_RENDERER_H
#define SHADER_WORKS_RENDERER_H

#include <stdbool.h>
#include <shader-works/maths.h>
#include <shader-works/shaders.h>
#include <shader-works/primitives.h>

// Handle restrict keyword for C++ compatibility
#ifdef __cplusplus
#define restrict
#endif

// Rendering constants
#define BASE_FOV (1.0472f) // 60 degrees in radians
#define FOV_OVER_2 (BASE_FOV / 2.0f) // 60 degrees in radians / 2
#define BASE_SCREEN_HEIGHT_WORLD (2.0f * tanf(FOV_OVER_2)) // Height of the view frustum at a distance of 1 unit

#ifndef UNUSED
#define UNUSED(X) (void)(X)
#endif

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
  u32 *skybox_buffer;   // panoramic skybox texture, client allocated

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
// skybox_buffer: pointer to pre-allocated panoramic skybox texture (u32 per pixel), or NULL
// width, height: dimensions of the framebuffer
// atlas_width, atlas_height: dimensions of the texture atlas
// max_depth: maximum depth value for depth buffering
void init_renderer(renderer_t *state, u32 win_width, u32 win_height, u32 atlas_width, u32 atlas_height, u32 *framebuffer, f32 *depthbuffer, u32 *skybox_buffer, f32 max_depth);

// Update camera basis vectors based on transform
// cam: pointer to camera transform
void update_camera(renderer_t *restrict state, transform_t *restrict cam);

// Render a model with given camera and lights
// state: pointer to renderer state
// cam: pointer to camera transform
// model: pointer to model to render
// lights: array of lights affecting the model
// light_count: number of lights in the array
// Returns the number of triangles rendered
usize render_model(renderer_t *state, transform_t *cam, model_t *model, light_t *lights, usize light_count);

// Render a single point in world space, applying transformations and depth testing
// Returns true if the point was drawn (not occluded), false if it was behind something
// state: pointer to renderer state
// cam: pointer to camera transform
// point: 3D position of the point in world space
// color: color of the point as a packed u32
bool render_point(renderer_t *restrict state, transform_t *restrict cam, float3 point, u32 color);

// Built-in effects

// TODO: Implement post processing shaders and convert this to new format
// Apply fog effect to the entire screen based on depth values
// fog_start: distance at which fog starts
// fog_end: distance at which fog fully obscures
// fog_r, fog_g, fog_b: RGB color of the fog
void apply_fog_to_screen(renderer_t *restrict state, f32 fog_start, f32 fog_end, u8 fog_r, u8 fog_g, u8 fog_b);

// Apply dithering effect to a color based on fragment coordinates and number of steps, used in fragment shaders to reduce color banding
u32 apply_dither_u32(u32 color, float2 frag_coord, float steps);

void transform_get_basis_vectors(transform_t *restrict t, float3 *restrict ihat, float3 *restrict jhat, float3 *restrict khat);
void transform_get_inverse_basis_vectors(transform_t *restrict t, float3 *restrict ihat, float3 *restrict jhat, float3 *restrict khat);

#endif // SHADER_WORKS_RENDERER_H
