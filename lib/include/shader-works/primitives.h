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
  float3 normal;
} vertex_data_t;

// Model structure with cache-friendly vertex layout
typedef struct {
  // Cache-friendly: all vertex data together
  vertex_data_t *vertex_data;
  float3 *face_normals;

  usize num_vertices;
  usize num_faces;

  float3 scale;
  transform_t transform;

  bool use_textures;
  bool disable_behind_camera_culling; // For particles that should render 360 degrees
  vertex_shader_t *vertex_shader;
  fragment_shader_t *frag_shader;
} model_t;

// Model generation functions

// Generates a plane centered at position with given size and segment size
// model: pointer to model structure to populate
// size: dimensions of the plane (width, height)
// segment_size: size of each segment (width, height)
// position: center position of the plane (x, y, z)
// Returns 0 on success, non-zero on failure
int generate_plane(model_t* model, float2 size, float2 segment_size, float3 position);

// Generates a cube centered at position with given size
// model: pointer to model structure to populate
// position: center position of the cube (x, y, z)
// size: dimensions of the cube (width, height, depth)
// Returns 0 on success, non-zero on failure
int generate_cube(model_t* model, float3 position, float3 size);

// Generates a sphere centered at position with given radius, segments, and rings
// model: pointer to model structure to populate
// radius: radius of the sphere
// segments: number of longitudinal segments
// rings: number of latitudinal rings
// position: center position of the sphere (x, y, z)
// Returns 0 on success, non-zero on failure
int generate_sphere(model_t* model, f32 radius, int segments, int rings, float3 position);

// Generates a quad at position with given size
// model: pointer to model structure to populate
// size: dimensions of the quad (width, height)
// position: center position of the quad (x, y, z)
// Returns 0 on success, non-zero on failure
int generate_quad(model_t* model, float2 size, float3 position);

// Model cleanup
void delete_model(model_t* model);

#endif // SHADER_WORKS_PRIMITIVES_H
