#ifndef SHADER_WORKS_PRIMITIVES_H
#define SHADER_WORKS_PRIMITIVES_H

#include <shader-works/maths.h>
#include <shader-works/shaders.h>

// Transform structure
typedef struct {
  f32 yaw;
  f32 pitch;
  float3 position;
} transform_t;

// Cache-friendly vertex data - groups related data together
typedef struct {
  float3 position;
  float2 uv;
  float3 normal;  // Per-vertex normal (if needed)
} vertex_data_t;

// Model structure with cache-friendly vertex layout
typedef struct {
  // Cache-friendly: all vertex data together
  vertex_data_t *vertex_data;  // Array of cache-friendly vertex structures
  float3 *face_normals;        // Face normals (one per triangle)

  usize num_vertices;
  usize num_faces;             // Number of triangle faces (num_vertices/3)

  float3 scale;
  transform_t transform;

  bool use_textures;
  vertex_shader_t *vertex_shader;
  fragment_shader_t *frag_shader;
} model_t;

// Model generation functions
int generate_plane(model_t* model, float2 size, float2 segment_size, float3 position);
int generate_cube(model_t* model, float3 position, float3 size);
int generate_sphere(model_t* model, f32 radius, int segments, int rings, float3 position);
int generate_billboard(model_t* model, float2 size, float3 position);

// Model cleanup
void delete_model(model_t* model);

#endif // SHADER_WORKS_PRIMITIVES_H