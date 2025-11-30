#include <math.h>
#include <stdlib.h>

#include <shader-works/maths.h>
#include <shader-works/primitives.h>

#include "scene.h"

#include "common/noise.h"

// Map a value from one range to another
inline float map_range(float value, float old_min, float old_max, float new_min, float new_max) {
  return new_min + (value - old_min) * (new_max - new_min) / (old_max - old_min);
}

// Terrain height function combining multiple noise layers for gradual hills and mountains
float terrainHeight(float x, float y, int seed) {
  // Define minimum height for frozen lakes
  const float lake_level = 0.0f;

  // Large-scale gentle rolling terrain (very low frequency for gradual hills)
  float large_hills = fbm(x * 0.003f, y * 0.003f, 4, g_world_config.seed + seed) * 40.0f;

  // Medium-scale terrain features
  float medium_hills = fbm(x * 0.008f, y * 0.008f, 5, g_world_config.seed + seed + 1) * 12.0f;

  // Small-scale detail
  float detail = fbm(x * 0.02f, y * 0.02f, 6, g_world_config.seed + seed + 2) * 4.0f;

  // Domain warping for more interesting mountain shapes
  float warp_x = fbm(x * 0.005f, y * 0.005f, 3, g_world_config.seed + seed + 3) * 20.0f;
  float warp_y = fbm(x * 0.005f, y * 0.005f, 3, g_world_config.seed + seed + 4) * 20.0f;

  // Mountain peaks using warped coordinates
  float mountains = ridgeNoise((x + warp_x) * 0.004f, (y + warp_y) * 0.004f, g_world_config.seed + seed + 5);
  mountains = powf(mountains, 1.5f) * 100.0f; // More dramatic peaks

  // Blend everything together with smooth transitions
  float base_terrain = large_hills + medium_hills * 0.7f + detail * 0.3f;

  // Shift terrain down to ensure some areas go below 0.0f for lakes
  base_terrain -= 8.0f; // This ensures low areas become lakes

  // Use the mountain noise as a mask to selectively add peaks
  float mountain_mask = smoothstep(fbm(x * 0.002f, y * 0.002f, 3, g_world_config.seed + seed + 6) * 0.5f + 0.5f);

  float height = base_terrain + mountains * mountain_mask;

  // Create frozen lakes by enforcing minimum height
  return fmaxf(height, lake_level);
}

float get_interpolated_terrain_height(float x, float z) {
  float grid_size = EPSILON;

  // Find which grid cell we're in
  float grid_x = x / grid_size;
  float grid_z = z / grid_size;

  // Get integer grid coordinates
  int gx = (int)floorf(grid_x);
  int gz = (int)floorf(grid_z);

  // Get fractional parts for interpolation
  float fx = grid_x - (float)gx;
  float fz = grid_z - (float)gz;

  // Sample terrain height at the four corners of the grid cell
  float corner_x0 = (float)gx * grid_size;
  float corner_x1 = (float)(gx + 1) * grid_size;
  float corner_z0 = (float)gz * grid_size;
  float corner_z1 = (float)(gz + 1) * grid_size;

  float h00 = terrainHeight(corner_x0, corner_z0, g_world_config.seed);
  float h10 = terrainHeight(corner_x1, corner_z0, g_world_config.seed);
  float h01 = terrainHeight(corner_x0, corner_z1, g_world_config.seed);
  float h11 = terrainHeight(corner_x1, corner_z1, g_world_config.seed);

  // Bilinear interpolation
  float h0 = lerp(h00, h10, fx);
  float h1 = lerp(h01, h11, fx);
  return lerp(h0, h1, fz);
}

void generate_ground_plane(model_t *model, float2 size, float2 segment_size, float3 position) {
  generate_plane(model, size, segment_size, position);
  model->transform = (transform_t){0};

  for (usize i = 0; i < model->num_vertices; ++i) {
    float3 *v = &model->vertex_data[i].position;
    v->y = terrainHeight(v->x, v->z, g_world_config.seed);
  }

  // Recalculate face normals after terrain height modification
  for (usize i = 0; i < model->num_faces; ++i) {
    usize base_idx = i * 3;
    float3 v0 = model->vertex_data[base_idx].position;
    float3 v1 = model->vertex_data[base_idx + 1].position;
    float3 v2 = model->vertex_data[base_idx + 2].position;

    float3 edge1 = float3_sub(v1, v0);
    float3 edge2 = float3_sub(v2, v0);

    model->face_normals[i] = float3_normalize(float3_cross(edge2, edge1));
  }

  // Recalculate vertex normals after terrain height modification
  for (usize i = 0; i < model->num_vertices; ++i) {
    model->vertex_data[i].normal = model->face_normals[i / 3];
  }
}
