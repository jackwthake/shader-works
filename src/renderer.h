#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>
#include "def.h"

#define WIN_WIDTH 400
#define WIN_HEIGHT 250

#define MAX_DEPTH 15.0f
#define FOG_START 5.0f
#define FOG_END   14.5f  // End of fog effect
#define FOG_R 22
#define FOG_G 35
#define FOG_B 65

#define PI 3.14159265f
#define EPSILON 0.0001f
#define fov_over_2 (1.0472f / 2.0f) // 60 degrees in radians / 2
#define ATLAS_WIDTH_PX 8
#define ATLAS_HEIGHT_PX 8

typedef struct {
  float3 world_pos;     // Interpolated world position of this specific pixel
  float2 screen_pos;    // Pixel coordinates on screen
  float2 uv;           // Texture coordinates
  float depth;         // Z-depth value (new_depth from render pipeline)
  float3 normal;       // Face normal
  float3 view_dir;     // Direction from fragment to camera
  float time;          // Frame time for animations
} shader_context_t;

typedef u32 (*shader_func)(u32 input_color, shader_context_t context, void *args, usize argc);

typedef struct {
  bool valid; // internally set
  usize argc;
  void *argv;
  shader_func func;
} shader_t;

typedef struct {
  f32 yaw;
  f32 pitch;
  float3 position;
} transform_t;

typedef struct {
  float3 *vertices; // List of vertices
  float2 *uvs; // pointer to texture coordinates (static data)
  float3 *face_normals; // Normals for each triangle face

  usize num_vertices;
  usize num_uvs;
  usize num_faces; // Number of triangle faces (num_vertices/3)
  
  float3 scale;
  transform_t transform;

  bool use_textures;
  shader_t *frag_shader;
} model_t;

extern shader_t default_shader;

u32 rgb_to_888(u8 r, u8 g, u8 b);

void init_renderer(game_state_t *state);
void update_camera(game_state_t *state, transform_t *cam);
usize render_model(game_state_t *state, transform_t *cam, model_t *model);

// --- Model Primitives ---
int generate_plane(model_t* model, float2 size, float2 segment_size, float3 position);
int generate_cube(model_t* model, float3 position, float3 size);
int generate_sphere(model_t* model, f32 radius, int segments, int rings, float3 position);

// --- shaders ---
shader_t make_shader(shader_func func, void *argv, usize argc);
void apply_fog_to_screen(game_state_t *state); // built in fog shader

#endif // RENDERER_H
