#ifndef CPU_RENDER_PRIMITIVES_H
#define CPU_RENDER_PRIMITIVES_H

#include "maths.h"
#include "shaders.h"

// Transform structure
typedef struct {
  f32 yaw;
  f32 pitch;
  float3 position;
} transform_t;

// Model structure
// TODO: struct of arrays better than array of structs for cache efficiency
typedef struct {
  float3 *vertices;        // List of vertices
  float2 *uvs;             // pointer to texture coordinates
  float3 *face_normals;    // Normals for each triangle face

  usize num_vertices;
  usize num_uvs;
  usize num_faces;         // Number of triangle faces (num_vertices/3)

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

#endif // CPU_RENDER_PRIMITIVES_H