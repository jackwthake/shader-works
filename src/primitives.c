#include <shader-works/primitives.h>

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>

int generate_plane(model_t* model, float2 size, float2 segment_size, float3 position) {
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
  // Since it's a flat plane, all face normals point upward
  float3 up_normal = { 0.0f, -1.0f, 0.0f };
  for (usize i = 0; i < total_triangles; i++) {
    model->face_normals[i] = up_normal;
  }

  free(grid_verts);
  free(grid_uvs);
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

int generate_billboard(model_t* model, float2 size, float3 position) {
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