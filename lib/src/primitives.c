#include <shader-works/primitives.h>

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <float.h>

int generate_plane(model_t* model, float2 size, float2 segment_size, float3 position) {
  return generate_plane_with_norm(model, size, segment_size, position, (float3){0.0f, -1.0f, 0.0f});
}

int generate_plane_with_norm(model_t* model, float2 size, float2 segment_size, float3 position, float3 normal) {
  if (!model || segment_size.x <= 0 || segment_size.y <= 0) return -1;

  // Calculate number of segments from size and segment_size
  usize w_segs = (usize)(size.x / segment_size.x);
  usize d_segs = (usize)(size.y / segment_size.y);

  usize w = w_segs + 1, d = d_segs + 1;
  usize grid_vertices = w * d;

  // Each quad becomes 2 triangles, so we need 6 vertices per quad
  usize num_quads = w_segs * d_segs;
  usize total_triangle_vertices = num_quads * 6;
  usize total_triangles = num_quads * 2; // Each quad has 2 triangles

  model->vertex_data = malloc(total_triangle_vertices * sizeof(vertex_data_t));
  model->face_normals = malloc(total_triangles * sizeof(float3));
  if (!model->vertex_data || !model->face_normals) {
    free(model->vertex_data);
    free(model->face_normals);
    return -1;
  }

  // Create temporary grid of vertices
  float3 *grid_verts = malloc(grid_vertices * sizeof(float3));
  float2 *grid_uvs = malloc(grid_vertices * sizeof(float2));
  if (!grid_verts || !grid_uvs) {
    free(model->vertex_data);
    free(model->face_normals);
    free(grid_verts);
    free(grid_uvs);
    return -1;
  }

  float wx = size.x / w_segs, dz = size.y / d_segs;
  float sx = position.x - size.x * 0.5f, sz = position.z - size.y * 0.5f;

  // Generate grid vertices
  for (usize z = 0; z < d; z++) {
    for (usize x = 0; x < w; x++) {
      usize i = z * w + x;
      grid_verts[i] = (float3){sx + x * wx, position.y, sz + z * dz};
      grid_uvs[i] = (float2){(float)x / w_segs, (float)z / d_segs};
    }
  }

  // Generate triangles from grid (CCW winding)
  usize vertex_idx = 0;
  for (usize z = 0; z < d_segs; z++) {
    for (usize x = 0; x < w_segs; x++) {
      // Get the four corners of current quad
      usize tl = z * w + x;           // top-left
      usize tr = z * w + (x + 1);     // top-right
      usize bl = (z + 1) * w + x;     // bottom-left
      usize br = (z + 1) * w + (x + 1); // bottom-right

      // First triangle: TL -> BL -> TR (CCW)
      model->vertex_data[vertex_idx] = (vertex_data_t){grid_verts[tl], grid_uvs[tl], {0.0f, -1.0f, 0.0f}};
      vertex_idx++;

      model->vertex_data[vertex_idx] = (vertex_data_t){grid_verts[bl], grid_uvs[bl], {0.0f, -1.0f, 0.0f}};
      vertex_idx++;

      model->vertex_data[vertex_idx] = (vertex_data_t){grid_verts[tr], grid_uvs[tr], {0.0f, -1.0f, 0.0f}};
      vertex_idx++;

      // Second triangle: TR -> BL -> BR (CCW)
      model->vertex_data[vertex_idx] = (vertex_data_t){grid_verts[tr], grid_uvs[tr], {0.0f, -1.0f, 0.0f}};
      vertex_idx++;

      model->vertex_data[vertex_idx] = (vertex_data_t){grid_verts[bl], grid_uvs[bl], {0.0f, -1.0f, 0.0f}};
      vertex_idx++;

      model->vertex_data[vertex_idx] = (vertex_data_t){grid_verts[br], grid_uvs[br], {0.0f, -1.0f, 0.0f}};
      vertex_idx++;
    }
  }

  model->num_vertices = total_triangle_vertices;
  model->num_faces = total_triangles;
  model->scale = (float3){1.0f, 1.0f, 1.0f};

  // Set up face normals for the plane
  // Since it's a flat plane, all face normals point in the given normal direction
  for (usize i = 0; i < total_triangles; i++) {
    model->face_normals[i] = normal;
  }

  free(grid_verts);
  free(grid_uvs);
  model->disable_behind_camera_culling = false;
  return 0;
}

int generate_cube(model_t* model, float3 position, float3 size) {
  if (!model) return -1;

  const int CUBE_VERTS = 36;  // 6 faces * 2 triangles * 3 vertices

  // Allocate cache-friendly vertex data and face normals
  model->vertex_data = malloc(CUBE_VERTS * sizeof(vertex_data_t));
  model->face_normals = malloc((CUBE_VERTS / 3) * sizeof(float3));

  if (!model->vertex_data || !model->face_normals) {
    free(model->vertex_data);
    free(model->face_normals);
    return -1;
  }

  // Half extents for more intuitive vertex positioning
  float3 half = {size.x * 0.5f, size.y * 0.5f, size.z * 0.5f};

  // Generate cache-friendly vertex data for each face
  int v = 0;  // vertex index

  // Front face (-Z, closest to camera)
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y, -half.z}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y, -half.z}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y, -half.z}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y, -half.z}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y, -half.z}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y, -half.z}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};

  // Back face (+Z, furthest from camera)
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y,  half.z}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y,  half.z}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y,  half.z}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y,  half.z}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y,  half.z}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y,  half.z}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}};

  // Left face (+X)
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y, -half.z}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y,  half.z}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y,  half.z}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y, -half.z}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y,  half.z}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y, -half.z}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}};

  // Right face (-X)
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y,  half.z}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y, -half.z}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y, -half.z}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y,  half.z}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y, -half.z}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y,  half.z}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};

  // Top face (+Y)
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y, -half.z}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y, -half.z}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y,  half.z}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y, -half.z}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y,  half.z}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y,  half.z}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}};

  // Bottom face (-Y)
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y,  half.z}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y,  half.z}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y, -half.z}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y,  half.z}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y, -half.z}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y, -half.z}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}};

  // Generate face normals - one normal per triangle
  for (int i = 0; i < CUBE_VERTS / 3; i++) {
    // Corrected face normals for -Y up, -Z forward coordinate system
    float3 normal;
    if (i < 2)       normal = (float3){0.0f, 0.0f, 1.0f};   // Front face (-Z) - normal points toward camera
    else if (i < 4)  normal = (float3){0.0f, 0.0f, -1.0f};  // Back face (+Z) - normal points away
    else if (i < 6)  normal = (float3){-1.0f, 0.0f, 0.0f};  // Left face (+X) - normal points left
    else if (i < 8)  normal = (float3){1.0f, 0.0f, 0.0f};   // Right face (-X) - normal points right
    else if (i < 10) normal = (float3){0.0f, -1.0f, 0.0f};  // Top face (+Y) - normal points down (up in -Y system)
    else             normal = (float3){0.0f, 1.0f, 0.0f};   // Bottom face (-Y) - normal points up (down in -Y system)

    model->face_normals[i] = normal;
  }

  // Set up model parameters
  model->num_vertices = CUBE_VERTS;
  model->num_faces = CUBE_VERTS / 3;
  model->scale = size;  // Store original size in scale

  // Initialize the transform
  model->transform.position = position;
  model->transform.yaw = 0.0f;
  model->transform.pitch = 0.0f;

  return 0;
}

int generate_inward_cube(model_t* model, float3 position, float3 size) {
  if (!model) return -1;

  const int CUBE_VERTS = 36;  // 6 faces * 2 triangles * 3 vertices

  model->vertex_data = malloc(CUBE_VERTS * sizeof(vertex_data_t));
  model->face_normals = malloc((CUBE_VERTS / 3) * sizeof(float3));

  if (!model->vertex_data || !model->face_normals) {
    free(model->vertex_data);
    free(model->face_normals);
    return -1;
  }

  float3 half = {size.x * 0.5f, size.y * 0.5f, size.z * 0.5f};
  int v = 0;

  // Front face (-Z) - Winding flipped to be visible from inside
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y, -half.z}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y, -half.z}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y, -half.z}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y, -half.z}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y, -half.z}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y, -half.z}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}};

  // Back face (+Z) - Winding flipped to be visible from inside
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y,  half.z}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y,  half.z}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y,  half.z}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y,  half.z}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y,  half.z}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y,  half.z}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};

  // Left face (+X) - Winding flipped to be visible from inside
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y, -half.z}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y,  half.z}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y,  half.z}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y, -half.z}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y, -half.z}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y,  half.z}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};

  // Right face (-X) - Winding flipped to be visible from inside
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y,  half.z}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y, -half.z}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y, -half.z}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y,  half.z}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y,  half.z}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y, -half.z}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}};

  // Top face (+Y) - Winding flipped to be visible from inside
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y, -half.z}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y,  half.z}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y, -half.z}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y, -half.z}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x,  half.y,  half.z}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x,  half.y,  half.z}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}};

  // Bottom face (-Y) - Winding flipped to be visible from inside
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y,  half.z}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y, -half.z}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y,  half.z}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y,  half.z}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{-half.x, -half.y, -half.z}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}};
  model->vertex_data[v++] = (vertex_data_t){{ half.x, -half.y, -half.z}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}};

  // Generate face normals pointing INWARD to the center
for (int i = 0; i < CUBE_VERTS / 3; i++) {
  float3 normal;
  if (i < 2)       normal = (float3){0.0f, 0.0f, 1.0f};   // Front wall points In (+Z)
  else if (i < 4)  normal = (float3){0.0f, 0.0f, -1.0f};  // Back wall points In (-Z)
  else if (i < 6)  normal = (float3){-1.0f, 0.0f, 0.0f};  // Left wall points In (-X)
  else if (i < 8)  normal = (float3){1.0f, 0.0f, 0.0f};   // Right wall points In (+X)
  else if (i < 10) normal = (float3){0.0f, -1.0f, 0.0f};  // Ceiling points In (-Y)
  else             normal = (float3){0.0f, 1.0f, 0.0f};   // Floor points In (+Y)

  model->face_normals[i] = normal;
}

  model->num_vertices = CUBE_VERTS;
  model->num_faces = CUBE_VERTS / 3;
  model->scale = size;
  model->transform.position = position;
  model->transform.yaw = 0.0f;
  model->transform.pitch = 0.0f;

  return 0;
}

int generate_sphere(model_t* model, f32 radius, int segments, int rings, float3 position) {
  assert(model != NULL);
  assert(segments >= 3);
  assert(rings >= 2);

  // Calculate number of vertices and UVs needed
  int num_vertices = (rings + 1) * (segments + 1);
  int num_triangles = 2 * rings * segments;

  // Allocate cache-friendly vertex data and face normals
  model->vertex_data = (vertex_data_t*)malloc(num_triangles * 3 * sizeof(vertex_data_t)); // 3 vertices per triangle
  model->face_normals = (float3*)malloc(num_triangles * sizeof(float3)); // 1 normal per triangle

  if (!model->vertex_data || !model->face_normals) {
    if (model->vertex_data) free(model->vertex_data);
    if (model->face_normals) free(model->face_normals);
    return -1;
  }

  // Generate temporary vertex and UV grids
  float3* temp_vertices = (float3*)malloc(num_vertices * sizeof(float3));
  float2* temp_uvs = (float2*)malloc(num_vertices * sizeof(float2));

  if (!temp_vertices || !temp_uvs) {
    free(model->vertex_data);
    free(model->face_normals);
    if (temp_vertices) free(temp_vertices);
    if (temp_uvs) free(temp_uvs);
    return -1;
  }

  // Generate vertices
  for (int ring = 0; ring <= rings; ring++) {
    f32 phi = PI * ((f32)ring / (f32)rings); // Latitude [0, PI]

    for (int segment = 0; segment <= segments; segment++) {
      f32 theta = 2.0f * PI * ((f32)segment / (f32)segments); // Longitude [0, 2PI]

      int idx = ring * (segments + 1) + segment;

      // Calculate normalized direction vector (also the normal direction)
      f32 sin_phi = sinf(phi);
      f32 cos_phi = cosf(phi);
      f32 sin_theta = sinf(theta);
      f32 cos_theta = cosf(theta);

      float3 normal = {
        sin_phi * cos_theta,
        cos_phi,
        sin_phi * sin_theta
      };

      // Scale by radius to get vertex position
      temp_vertices[idx].x = normal.x * radius;
      temp_vertices[idx].y = normal.y * radius;
      temp_vertices[idx].z = normal.z * radius;

      // Generate UVs
      temp_uvs[idx].x = (f32)segment / (f32)segments;
      temp_uvs[idx].y = (f32)ring / (f32)rings;
    }
  }

  // Generate triangles and face normals
  int vertex_index = 0;
  int face_index = 0;

  for (int ring = 0; ring < rings; ring++) {
    for (int segment = 0; segment < segments; segment++) {
      int current = ring * (segments + 1) + segment;
      int next = current + segments + 1;

      // --- First triangle of quad ---
      float3 v0 = temp_vertices[current];
      float3 v1 = temp_vertices[next];
      float3 v2 = temp_vertices[current + 1];

      // Calculate normals for sphere (pointing outward)
      float3 n0 = float3_normalize(v0);
      float3 n1 = float3_normalize(v1);
      float3 n2 = float3_normalize(v2);

      model->vertex_data[vertex_index]     = (vertex_data_t){v0, temp_uvs[current], n0};
      model->vertex_data[vertex_index + 1] = (vertex_data_t){v1, temp_uvs[next], n1};
      model->vertex_data[vertex_index + 2] = (vertex_data_t){v2, temp_uvs[current + 1], n2};

      float3 edge1 = float3_sub(v1, v0);
      float3 edge2 = float3_sub(v2, v0);
      model->face_normals[face_index] = float3_normalize(float3_cross(edge1, edge2));
      face_index++;

      // --- Second triangle of quad ---
      float3 v4 = temp_vertices[next + 1];

      // Calculate normals for sphere (pointing outward)
      float3 n4 = float3_normalize(v4);

      model->vertex_data[vertex_index + 3] = (vertex_data_t){v1, temp_uvs[next], n1};
      model->vertex_data[vertex_index + 4] = (vertex_data_t){v4, temp_uvs[next + 1], n4};
      model->vertex_data[vertex_index + 5] = (vertex_data_t){v2, temp_uvs[current + 1], n2};

      edge1 = float3_sub(v4, v1);
      edge2 = float3_sub(v2, v1);
      model->face_normals[face_index] = float3_normalize(float3_cross(edge1, edge2));
      face_index++;

      vertex_index += 6;
    }
  }

  free(temp_vertices);
  free(temp_uvs);

  model->num_vertices = num_triangles * 3;
  model->num_faces = num_triangles;

  model->transform.position = position;
  model->transform.yaw = 0.0f;
  model->transform.pitch = 0.0f;
  model->scale = make_float3(1.0f, 1.0f, 1.0f);

  return 0;
}

int generate_quad(model_t* model, float2 size, float3 position) {
  if (!model) return -1;

  const int BILLBOARD_VERTS = 6;  // 2 triangles * 3 vertices each

  // Allocate cache-friendly vertex data and face normals
  model->vertex_data = malloc(BILLBOARD_VERTS * sizeof(vertex_data_t));
  model->face_normals = malloc(2 * sizeof(float3));  // 2 triangles

  if (!model->vertex_data || !model->face_normals) {
    free(model->vertex_data);
    free(model->face_normals);
    return -1;
  }

  // Half extents for quad positioning
  float half_width = size.x * 0.5f;
  float half_height = size.y * 0.5f;

  // Generate cache-friendly vertex data facing the camera
  float3 normal = {0.0f, 0.0f, -1.0f}; // Face normal toward camera

  // Triangle 1: counter-clockwise winding when viewed from camera position
  model->vertex_data[0] = (vertex_data_t){{-half_width, -half_height, 0.0f}, {0.0f, 1.0f}, normal};
  model->vertex_data[1] = (vertex_data_t){{ half_width, -half_height, 0.0f}, {1.0f, 1.0f}, normal};
  model->vertex_data[2] = (vertex_data_t){{-half_width,  half_height, 0.0f}, {0.0f, 0.0f}, normal};

  // Triangle 2: counter-clockwise winding when viewed from camera position
  model->vertex_data[3] = (vertex_data_t){{ half_width, -half_height, 0.0f}, {1.0f, 1.0f}, normal};
  model->vertex_data[4] = (vertex_data_t){{ half_width,  half_height, 0.0f}, {1.0f, 0.0f}, normal};
  model->vertex_data[5] = (vertex_data_t){{-half_width,  half_height, 0.0f}, {0.0f, 0.0f}, normal};

  // Face normals point toward camera (camera looks down -Z, so billboard faces -Z)
  model->face_normals[0] = normal;
  model->face_normals[1] = normal;

  // Set up model parameters
  model->num_vertices = BILLBOARD_VERTS;
  model->num_faces = 2;
  model->scale = (float3){1.0f, 1.0f, 1.0f};
  model->disable_behind_camera_culling = false;

  // Initialize the transform
  model->transform.position = position;
  model->transform.yaw = 0.0f;
  model->transform.pitch = 0.0f;

  return 0;
}

void delete_model(model_t* model) {
  if (!model) return;

  if (model->vertex_data) {
    free(model->vertex_data);
    model->vertex_data = NULL;
  }

  if (model->face_normals) {
    free(model->face_normals);
    model->face_normals = NULL;
  }

  model->num_vertices = 0;
  model->num_faces = 0;

  model->frag_shader = NULL;
  model->vertex_shader = NULL;

  model->use_textures = false;
}

// ============================================================================
// OBJ Model Loader
// ============================================================================

#define OBJ_INITIAL_CAPACITY 1024

typedef struct {
  float3 *positions;
  float2 *texcoords;
  float3 *normals;
  usize pos_count, tex_count, norm_count;
  usize pos_capacity, tex_capacity, norm_capacity;
} obj_buffers_t;

static int obj_parse_face_vertex(const char* vert_str, int *v_idx, int *vt_idx, int *vn_idx) {
  // Parse formats: v, v/vt, v//vn, v/vt/vn
  int result = sscanf(vert_str, "%d/%d/%d", v_idx, vt_idx, vn_idx);

  if (result == 1) {
    // Format: v
    *vt_idx = -1;
    *vn_idx = -1;
  } else if (result == 2) {
    // Format: v/vt
    *vn_idx = -1;
  } else if (result != 3) {
    // Try v//vn format
    result = sscanf(vert_str, "%d//%d", v_idx, vn_idx);
    if (result == 2) {
      *vt_idx = -1;
    } else {
      return -1; // Parse error
    }
  }

  return 0;
}

int load_obj_model(model_t* model, const char* filename, float3 position, float scale, bool flip_winding) {
  if (!model || !filename) return -1;

  FILE *file = fopen(filename, "r");
  if (!file) {
    return -1;
  }

  obj_buffers_t buffers = {0};
  buffers.pos_capacity = OBJ_INITIAL_CAPACITY;
  buffers.tex_capacity = OBJ_INITIAL_CAPACITY;
  buffers.norm_capacity = OBJ_INITIAL_CAPACITY;

  buffers.positions = malloc(buffers.pos_capacity * sizeof(float3));
  buffers.texcoords = malloc(buffers.tex_capacity * sizeof(float2));
  buffers.normals = malloc(buffers.norm_capacity * sizeof(float3));

  if (!buffers.positions || !buffers.texcoords || !buffers.normals) {
    free(buffers.positions);
    free(buffers.texcoords);
    free(buffers.normals);
    fclose(file);
    return -1;
  }

  // Dynamic array for face indices (each face can have 3+ vertices)
  typedef struct { int v, vt, vn; } face_vertex_t;
  face_vertex_t *face_vertices = malloc(OBJ_INITIAL_CAPACITY * sizeof(face_vertex_t));
  usize face_vert_capacity = OBJ_INITIAL_CAPACITY;
  usize face_vert_count = 0;

  usize num_triangles = 0;
  char line[512];
  float3 default_normal = {0.0f, 1.0f, 0.0f};
  float2 default_uv = {0.0f, 0.0f};

  // First pass: parse all data
  while (fgets(line, sizeof(line), file)) {
    // Skip whitespace
    char *p = line;
    while (*p == ' ' || *p == '\t') p++;

    if (p[0] == 'v' && (p[1] == ' ' || p[1] == '\t')) {
      // Vertex position
      float x, y, z;
      if (sscanf(p + 1, "%f %f %f", &x, &y, &z) == 3) {
        if (buffers.pos_count >= buffers.pos_capacity) {
          buffers.pos_capacity *= 2;
          float3 *temp = realloc(buffers.positions, buffers.pos_capacity * sizeof(float3));
          if (!temp) goto cleanup_error;
          buffers.positions = temp;
        }
        // Flip Z to convert from OBJ (+Z forward) to render system (-Z forward)
        // Don't scale here - let transform.scale handle it
        buffers.positions[buffers.pos_count++] = (float3){x, y, -z};
      }
    }
    else if (p[0] == 'v' && p[1] == 't' && (p[2] == ' ' || p[2] == '\t')) {
      // Texture coordinate
      float u, v;
      if (sscanf(p + 2, "%f %f", &u, &v) >= 1) {
        if (buffers.tex_count >= buffers.tex_capacity) {
          buffers.tex_capacity *= 2;
          float2 *temp = realloc(buffers.texcoords, buffers.tex_capacity * sizeof(float2));
          if (!temp) goto cleanup_error;
          buffers.texcoords = temp;
        }
        float v_coord = (sscanf(p + 2, "%f %f", &u, &v) == 2) ? v : 0.0f;
        buffers.texcoords[buffers.tex_count++] = (float2){u, v_coord};
      }
    }
    else if (p[0] == 'v' && p[1] == 'n' && (p[2] == ' ' || p[2] == '\t')) {
      // Vertex normal
      float nx, ny, nz;
      if (sscanf(p + 2, "%f %f %f", &nx, &ny, &nz) == 3) {
        if (buffers.norm_count >= buffers.norm_capacity) {
          buffers.norm_capacity *= 2;
          float3 *temp = realloc(buffers.normals, buffers.norm_capacity * sizeof(float3));
          if (!temp) goto cleanup_error;
          buffers.normals = temp;
        }
        // Flip all normal components for correct lighting with -Z forward camera
        buffers.normals[buffers.norm_count++] = (float3){-nx, -ny, -nz};
      }
    }
    else if (p[0] == 'f' && (p[1] == ' ' || p[1] == '\t')) {
      // Face - can have 3+ vertices (triangulate n-gons)
      face_vert_count = 0;
      char *line_ptr = p + 2;  // Skip "f "

      // Skip leading whitespace
      while (*line_ptr && (*line_ptr == ' ' || *line_ptr == '\t')) line_ptr++;

      // Parse all vertices in this face
      while (*line_ptr && *line_ptr != '\n' && *line_ptr != '#') {
        if (face_vert_count >= face_vert_capacity) {
          face_vert_capacity *= 2;
          face_vertex_t *temp = realloc(face_vertices, face_vert_capacity * sizeof(face_vertex_t));
          if (!temp) goto cleanup_error;
          face_vertices = temp;
        }

        int v, vt, vn;
        int bytes_read;

        // Try to parse vertex specification
        if (sscanf(line_ptr, "%d/%d/%d%n", &v, &vt, &vn, &bytes_read) == 3) {
          // Full format: v/vt/vn
          line_ptr += bytes_read;
        } else if (sscanf(line_ptr, "%d//%d%n", &v, &vn, &bytes_read) == 2) {
          // Format: v//vn
          vt = -1;
          line_ptr += bytes_read;
        } else if (sscanf(line_ptr, "%d/%d%n", &v, &vt, &bytes_read) == 2) {
          // Format: v/vt
          vn = -1;
          line_ptr += bytes_read;
        } else if (sscanf(line_ptr, "%d%n", &v, &bytes_read) == 1) {
          // Format: v
          vt = -1;
          vn = -1;
          line_ptr += bytes_read;
        } else {
          // Parse error - skip this character and continue
          line_ptr++;
          continue;
        }

        // Convert to 0-indexed
        face_vertices[face_vert_count].v = (v > 0) ? v - 1 : -1;
        face_vertices[face_vert_count].vt = (vt > 0) ? vt - 1 : -1;
        face_vertices[face_vert_count].vn = (vn > 0) ? vn - 1 : -1;
        face_vert_count++;

        // Skip whitespace to next vertex
        while (*line_ptr && (*line_ptr == ' ' || *line_ptr == '\t')) line_ptr++;
      }

      // Triangulate n-gon using fan triangulation
      if (face_vert_count >= 3) {
        for (usize tri = 1; tri < face_vert_count - 1; tri++) {
          num_triangles++;
        }
      }
    }
  }

  fprintf(stderr, "[OBJ] First pass: counted %zu triangles\n", num_triangles);
  fflush(stderr);

  // Calculate model centroid to center it at origin
  float3 min_bound = {FLT_MAX, FLT_MAX, FLT_MAX};
  float3 max_bound = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

  for (usize i = 0; i < buffers.pos_count; i++) {
    if (buffers.positions[i].x < min_bound.x) min_bound.x = buffers.positions[i].x;
    if (buffers.positions[i].y < min_bound.y) min_bound.y = buffers.positions[i].y;
    if (buffers.positions[i].z < min_bound.z) min_bound.z = buffers.positions[i].z;
    if (buffers.positions[i].x > max_bound.x) max_bound.x = buffers.positions[i].x;
    if (buffers.positions[i].y > max_bound.y) max_bound.y = buffers.positions[i].y;
    if (buffers.positions[i].z > max_bound.z) max_bound.z = buffers.positions[i].z;
  }

  float3 centroid = {
    (min_bound.x + max_bound.x) * 0.5f,
    (min_bound.y + max_bound.y) * 0.5f,
    (min_bound.z + max_bound.z) * 0.5f
  };

  // Center all vertices
  for (usize i = 0; i < buffers.pos_count; i++) {
    buffers.positions[i].x -= centroid.x;
    buffers.positions[i].y -= centroid.y;
    buffers.positions[i].z -= centroid.z;
  }

  // Allocate vertex and face data
  usize total_vertices = num_triangles * 3;
  model->vertex_data = malloc(total_vertices * sizeof(vertex_data_t));
  model->face_normals = malloc(num_triangles * sizeof(float3));

  if (!model->vertex_data || !model->face_normals) {
    goto cleanup_error;
  }

  // Second pass: reconstruct vertex data with proper indexing
  rewind(file);
  usize vert_idx = 0;
  usize face_idx = 0;

  while (fgets(line, sizeof(line), file)) {
    char *p = line;
    while (*p == ' ' || *p == '\t') p++;

    if (p[0] == 'f' && (p[1] == ' ' || p[1] == '\t')) {
      face_vert_count = 0;
      char *line_ptr = p + 2;  // Skip "f "

      // Skip leading whitespace
      while (*line_ptr && (*line_ptr == ' ' || *line_ptr == '\t')) line_ptr++;

      // Parse all vertices in this face
      while (*line_ptr && *line_ptr != '\n' && *line_ptr != '#') {
        if (face_vert_count >= face_vert_capacity) break;

        int v, vt, vn;
        int bytes_read;

        // Try to parse vertex specification
        if (sscanf(line_ptr, "%d/%d/%d%n", &v, &vt, &vn, &bytes_read) == 3) {
          // Full format: v/vt/vn
          line_ptr += bytes_read;
        } else if (sscanf(line_ptr, "%d//%d%n", &v, &vn, &bytes_read) == 2) {
          // Format: v//vn
          vt = -1;
          line_ptr += bytes_read;
        } else if (sscanf(line_ptr, "%d/%d%n", &v, &vt, &bytes_read) == 2) {
          // Format: v/vt
          vn = -1;
          line_ptr += bytes_read;
        } else if (sscanf(line_ptr, "%d%n", &v, &bytes_read) == 1) {
          // Format: v
          vt = -1;
          vn = -1;
          line_ptr += bytes_read;
        } else {
          // Parse error - skip this character and continue
          line_ptr++;
          continue;
        }

        // Convert to 0-indexed
        face_vertices[face_vert_count].v = (v > 0) ? v - 1 : -1;
        face_vertices[face_vert_count].vt = (vt > 0) ? vt - 1 : -1;
        face_vertices[face_vert_count].vn = (vn > 0) ? vn - 1 : -1;
        face_vert_count++;

        // Skip whitespace to next vertex
        while (*line_ptr && (*line_ptr == ' ' || *line_ptr == '\t')) line_ptr++;
      }

      // Triangulate and generate vertex data
      for (usize tri = 1; tri < face_vert_count - 1; tri++) {
        // Fan triangulation: vertex 0, tri, tri+1
        usize tri_indices[3];
        if (flip_winding) {
          tri_indices[0] = 0;
          tri_indices[1] = tri + 1;
          tri_indices[2] = tri;
        } else {
          tri_indices[0] = 0;
          tri_indices[1] = tri;
          tri_indices[2] = tri + 1;
        }

        // Process three vertices of the triangle
        for (int i = 0; i < 3; i++) {
          usize face_vert_idx = tri_indices[i];
          if (face_vert_idx >= face_vert_count) continue;

          int v_idx = face_vertices[face_vert_idx].v;
          int vt_idx = face_vertices[face_vert_idx].vt;
          int vn_idx = face_vertices[face_vert_idx].vn;

          if (v_idx < 0 || (usize)v_idx >= buffers.pos_count) {
            goto cleanup_error;
          }

          // Don't bake position offset into vertices - store it in transform instead
          float3 pos = buffers.positions[v_idx];

          float2 uv = (vt_idx >= 0 && (usize)vt_idx < buffers.tex_count)
                      ? buffers.texcoords[vt_idx]
                      : default_uv;

          float3 norm = (vn_idx >= 0 && (usize)vn_idx < buffers.norm_count)
                        ? buffers.normals[vn_idx]
                        : default_normal;

          if (vert_idx < total_vertices) {
            model->vertex_data[vert_idx] = (vertex_data_t){pos, uv, norm};
            vert_idx++;
          }
        }

        // Compute and store face normal (for back-face culling)
        if (face_idx < num_triangles && vert_idx >= 3) {
          float3 v0 = model->vertex_data[vert_idx - 3].position;
          float3 v1 = model->vertex_data[vert_idx - 2].position;
          float3 v2 = model->vertex_data[vert_idx - 1].position;

          float3 e1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
          float3 e2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};

          // Cross product: e1 × e2 (order matters for normal direction)
          // Since we may have flipped winding and Z coordinates, this gives us the correct outward normal
          float3 normal = {
            e1.y * e2.z - e1.z * e2.y,
            e1.z * e2.x - e1.x * e2.z,
            e1.x * e2.y - e1.y * e2.x
          };

          // Normalize
          float len = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
          if (len > 0.0001f) {
            normal.x /= len;
            normal.y /= len;
            normal.z /= len;
          } else {
            // Degenerate triangle, use default
            normal = (float3){0.0f, 1.0f, 0.0f};
          }

          model->face_normals[face_idx] = normal;
          face_idx++;
        }
      }
    }
  }

  model->num_vertices = vert_idx;
  model->num_faces = face_idx;
  model->scale = (float3){scale, scale, scale};
  model->transform.position = position;
  model->transform.yaw = 0.0f;
  model->transform.pitch = 0.0f;
  model->transform.roll = 0.0f;
  model->use_textures = (buffers.tex_count > 0);
  model->disable_behind_camera_culling = false;
  model->vertex_shader = NULL;
  model->frag_shader = NULL;

  // Cleanup
  free(buffers.positions);
  free(buffers.texcoords);
  free(buffers.normals);
  free(face_vertices);
  fclose(file);

  return 0;

cleanup_error:
  free(buffers.positions);
  free(buffers.texcoords);
  free(buffers.normals);
  free(face_vertices);
  fclose(file);
  return -1;
}
