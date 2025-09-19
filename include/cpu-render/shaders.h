#ifndef CPU_RENDER_SHADERS_H
#define CPU_RENDER_SHADERS_H

#include <stdbool.h>
#include "maths.h"

// Shader context structures
typedef struct {
  float3 world_pos;     // Interpolated world position of this specific pixel
  float2 screen_pos;    // Pixel coordinates on screen
  float2 uv;           // Texture coordinates
  float depth;         // Z-depth value (new_depth from render pipeline)
  float3 normal;       // Face normal
  float3 view_dir;     // Direction from fragment to camera
  float time;          // Frame time for animations
} fragment_context_t;

typedef struct {
  // Camera information
  float3 cam_position;      // Camera world position
  float3 cam_forward;       // Camera forward vector
  float3 cam_right;         // Camera right vector
  float3 cam_up;            // Camera up vector

  // Projection parameters
  float projection_scale;   // For perspective projection
  float frustum_bound;      // View frustum boundaries
  float2 screen_dim;        // Screen dimensions

  // Timing
  float time;               // Current time

  // Per-vertex data
  int vertex_index;         // Which vertex in the model (0, 1, 2 within triangle)
  int triangle_index;       // Which triangle this vertex belongs to

  // Original vertex data
  float3 original_vertex;   // Original vertex position in model space
  float2 original_uv;       // Original UV coordinates
} vertex_context_t;

// Shader function pointers
typedef u32 (*fragment_shader_func)(u32 input_color, fragment_context_t context, void *args, usize argc);
typedef float3 (*vertex_shader_func)(vertex_context_t context, void *args, usize argc);

// Shader structures
typedef struct {
  bool valid;
  usize argc;
  void *argv;
  fragment_shader_func func;
} fragment_shader_t;

typedef struct {
  bool valid;
  usize argc;
  void *argv;
  vertex_shader_func func;
} vertex_shader_t;

// Shader creation functions
vertex_shader_t make_vertex_shader(vertex_shader_func func, void *argv, usize argc);
fragment_shader_t make_fragment_shader(fragment_shader_func func, void *argv, usize argc);

// Built-in shaders
extern vertex_shader_t default_vertex_shader;
extern fragment_shader_t default_frag_shader;

#endif // CPU_RENDER_SHADERS_H