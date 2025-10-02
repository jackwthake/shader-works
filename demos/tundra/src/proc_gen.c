#include <math.h>
#include <stdlib.h>

#include <shader-works/maths.h>
#include <shader-works/primitives.h>

#include "scene.h"

// Smooth step function for better interpolation
static inline float smoothstep(float t) {
  return t * t * (3.0f - 2.0f * t);
}

// Map a value from one range to another
inline float map_range(float value, float old_min, float old_max, float new_min, float new_max) {
  return new_min + (value - old_min) * (new_max - new_min) / (old_max - old_min);
}

// Simple 1D interpolation
inline float lerp(float a, float b, float t) {
  return a + t * (b - a);
}

// Hash function to get pseudo-random gradients with seed
inline float hash2(int x, int y, int seed) {
  int n = x + y * 57 + seed * 2654435761;
  n = (n << 13) ^ n;
  return (1.0f - (float)((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

// Simple 2D Perlin-like noise (using a hash for gradients) with seed
float noise2D(float x, float y, int seed) {
  // Grid coordinates - use floor for proper handling of negative coordinates
  int xi = (int)floorf(x);
  int yi = (int)floorf(y);

  // Fractional parts
  float xf = x - (float)xi;
  float yf = y - (float)yi;
  
  // Hash function to get pseudo-random gradients at grid points
  // (This is simplified - you'd want better hashing in practice)
  float a = hash2(xi, yi, seed);
  float b = hash2(xi + 1, yi, seed);
  float c = hash2(xi, yi + 1, seed);
  float d = hash2(xi + 1, yi + 1, seed);
  
  // Interpolate
  float i1 = lerp(a, b, smoothstep(xf));
  float i2 = lerp(c, d, smoothstep(xf));
  return lerp(i1, i2, smoothstep(yf));
}

// Fractal Brownian Motion (fBm) for generating terrain with seed
static float fbm(float x, float y, int octaves, int seed) {
  float value = 0.0f;
  float amplitude = 1.0f;
  float frequency = 1.0f;
  float maxValue = 0.0f;  // For normalizing
  
  for (int i = 0; i < octaves; i++) {
    value += noise2D(x * frequency, y * frequency, seed + i) * amplitude;
    maxValue += amplitude;
    
    amplitude *= 0.5f;  // Each octave has half the amplitude
    frequency *= 2.0f;  // Each octave has double the frequency
  }
  
  return value / maxValue;  // Normalize to [-1, 1]
}

// Ridge noise function to create sharp features with seed
float ridgeNoise(float x, float y, int seed) {
  float n = fbm(x, y, 4, seed);
  return 1.0f - fabsf(n);  // Creates ridges by inverting absolute value
}

// Terrain height function combining multiple noise layers for gradual hills and mountains
float terrainHeight(float x, float y, int seed) {
  // Define minimum height for frozen lakes
  const float lake_level = 0.0f;

  // Large-scale gentle rolling terrain (very low frequency for gradual hills)
  float large_hills = fbm(x * 0.003f, y * 0.003f, 4, g_world_config.seed + seed) * 25.0f;

  // Medium-scale terrain features
  float medium_hills = fbm(x * 0.008f, y * 0.008f, 5, g_world_config.seed + seed + 1) * 12.0f;

  // Small-scale detail
  float detail = fbm(x * 0.02f, y * 0.02f, 6, g_world_config.seed + seed + 2) * 4.0f;

  // Domain warping for more interesting mountain shapes
  float warp_x = fbm(x * 0.005f, y * 0.005f, 3, g_world_config.seed + seed + 3) * 20.0f;
  float warp_y = fbm(x * 0.005f, y * 0.005f, 3, g_world_config.seed + seed + 4) * 20.0f;

  // Mountain peaks using warped coordinates
  float mountains = ridgeNoise((x + warp_x) * 0.004f, (y + warp_y) * 0.004f, g_world_config.seed + seed + 5);
  mountains = powf(mountains, 1.5f) * 35.0f; // More dramatic peaks

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
  float grid_size = 1.0f; // Sample every 1 unit
  
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

// this should append each segment to the model's vertex_data and face_normals
int generate_tree_cylinder(model_t* model, float bottom_radius, float top_radius, float height, float3 bottom_center, float3 top_center, usize segments, float bottom_angle_offset, float top_angle_offset) {
  if (!model || segments < 4 || bottom_radius < 0 || top_radius < 0 || height <= 0.001f) return -1;

  // Skip degenerate cylinders where both radii are too small
  if (bottom_radius < 0.0001f && top_radius < 0.0001f) return 0;
  
  // Calculate vertices and faces
  usize side_vertices = segments * 6;         // 2 triangles per segment = 6 vertices
  usize bottom_cap_vertices = (segments - 2) * 3;
  usize top_cap_vertices = (segments - 2) * 3;
  usize new_vertices = side_vertices + bottom_cap_vertices + top_cap_vertices;
  usize new_faces = segments * 2 + (segments - 2) + (segments - 2);
  
  usize old_vertices = model->num_vertices;
  usize old_faces = model->num_faces;
  
  // Reallocate arrays for append
  vertex_data_t* tmp_v = realloc(model->vertex_data, (old_vertices + new_vertices) * sizeof(vertex_data_t));
  float3* tmp_f = realloc(model->face_normals, (old_faces + new_faces) * sizeof(float3));
  
  if (!tmp_v || !tmp_f) {
    free(tmp_v);
    free(tmp_f);
    return -1;
  }
  
  model->vertex_data = tmp_v;
  model->face_normals = tmp_f;
  
  float3 axis = float3_normalize(float3_sub(top_center, bottom_center));
  
  usize vertex_idx = old_vertices;
  usize face_idx = old_faces;
  
  // -------- Side faces --------
  for (usize i = 0; i < segments; i++) {
    // Bottom angles
    float angle1b = (2.0f * PI * i) / segments + bottom_angle_offset;
    float angle2b = (2.0f * PI * (i + 1)) / segments + bottom_angle_offset;
    
    float cos1b = cosf(angle1b), sin1b = sinf(angle1b);
    float cos2b = cosf(angle2b), sin2b = sinf(angle2b);
    
    // Top angles
    float angle1t = (2.0f * PI * i) / segments + top_angle_offset;
    float angle2t = (2.0f * PI * (i + 1)) / segments + top_angle_offset;
    
    float cos1t = cosf(angle1t), sin1t = sinf(angle1t);
    float cos2t = cosf(angle2t), sin2t = sinf(angle2t);
    
    // Tangent basis
    float3 right = float3_normalize(float3_cross(axis, make_float3(0, 1, 0)));
    if (float3_magnitude(right) < 0.1f)
    right = float3_normalize(float3_cross(axis, make_float3(1, 0, 0)));
    float3 forward = float3_normalize(float3_cross(axis, right));
    
    // Bottom vertices
    float3 bottom1 = float3_add(bottom_center,
      float3_add(float3_scale(right, cos1b * bottom_radius),
      float3_scale(forward, sin1b * bottom_radius)));
      float3 bottom2 = float3_add(bottom_center,
      float3_add(float3_scale(right, cos2b * bottom_radius),
      float3_scale(forward, sin2b * bottom_radius)));
        
    // Top vertices
    float3 top1 = float3_add(top_center,
                  float3_add(float3_scale(right, cos1t * top_radius),
                  float3_scale(forward, sin1t * top_radius)));
    float3 top2 = float3_add(top_center,
                  float3_add(float3_scale(right, cos2t * top_radius),
                  float3_scale(forward, sin2t * top_radius)));
      
    // Side normal
    float3 edge1 = float3_sub(top1, bottom1);
    float3 edge2 = float3_sub(bottom2, bottom1);
    float3 normal = float3_normalize(float3_cross(edge1, edge2));
    
    // First triangle
    model->vertex_data[vertex_idx++] = (vertex_data_t){ bottom1, make_float2((float)i / segments, 0.0f), normal };
    model->vertex_data[vertex_idx++] = (vertex_data_t){ bottom2, make_float2((float)(i + 1) / segments, 0.0f), normal };
    model->vertex_data[vertex_idx++] = (vertex_data_t){ top1,    make_float2((float)i / segments, 1.0f), normal };
    
    // Second triangle
    model->vertex_data[vertex_idx++] = (vertex_data_t){ bottom2, make_float2((float)(i + 1) / segments, 0.0f), normal };
    model->vertex_data[vertex_idx++] = (vertex_data_t){ top2,    make_float2((float)(i + 1) / segments, 1.0f), normal };
    model->vertex_data[vertex_idx++] = (vertex_data_t){ top1,    make_float2((float)i / segments, 1.0f), normal };
    
    model->face_normals[face_idx++] = normal;
    model->face_normals[face_idx++] = normal;
  }
  
  // -------- Bottom cap --------
  if (bottom_radius > 0.0001f) {
    float3 bottom_normal = float3_scale(axis, -1.0f);
    for (usize i = 1; i < segments - 1; i++) {
      float angle0 = 0 + bottom_angle_offset;
      float angle1 = (2.0f * PI * i) / segments + bottom_angle_offset;
      float angle2 = (2.0f * PI * (i + 1)) / segments + bottom_angle_offset;
      
      float cos0 = cosf(angle0), sin0 = sinf(angle0);
      float cos1 = cosf(angle1), sin1 = sinf(angle1);
      float cos2 = cosf(angle2), sin2 = sinf(angle2);
      
      float3 right = float3_normalize(float3_cross(axis, make_float3(0, 1, 0)));
      if (float3_magnitude(right) < 0.1f)
      right = float3_normalize(float3_cross(axis, make_float3(1, 0, 0)));
      float3 forward = float3_normalize(float3_cross(axis, right));
      
      float3 v0 = float3_add(bottom_center,
                  float3_add(float3_scale(right, cos0 * bottom_radius),
                  float3_scale(forward, sin0 * bottom_radius)));
      float3 v1 = float3_add(bottom_center,
                  float3_add(float3_scale(right, cos1 * bottom_radius),
                  float3_scale(forward, sin1 * bottom_radius)));
      float3 v2 = float3_add(bottom_center,
          float3_add(float3_scale(right, cos2 * bottom_radius),
          float3_scale(forward, sin2 * bottom_radius)));
          
          model->vertex_data[vertex_idx++] = (vertex_data_t){ v0, make_float2(0.5f + cos0 * 0.5f, 0.5f + sin0 * 0.5f), bottom_normal };
          model->vertex_data[vertex_idx++] = (vertex_data_t){ v1, make_float2(0.5f + cos1 * 0.5f, 0.5f + sin1 * 0.5f), bottom_normal };
          model->vertex_data[vertex_idx++] = (vertex_data_t){ v2, make_float2(0.5f + cos2 * 0.5f, 0.5f + sin2 * 0.5f), bottom_normal };
          
          model->face_normals[face_idx++] = bottom_normal;
    }
  }
      
  // -------- Top cap --------
  if (top_radius > 0.0001f) {
    float3 top_normal = axis;
    for (usize i = 1; i < segments - 1; i++) {
      float angle0 = 0 + top_angle_offset;
      float angle1 = (2.0f * PI * i) / segments + top_angle_offset;
      float angle2 = (2.0f * PI * (i + 1)) / segments + top_angle_offset;
      
      float cos0 = cosf(angle0), sin0 = sinf(angle0);
      float cos1 = cosf(angle1), sin1 = sinf(angle1);
      float cos2 = cosf(angle2), sin2 = sinf(angle2);
      
      float3 right = float3_normalize(float3_cross(axis, make_float3(0, 1, 0)));
      if (float3_magnitude(right) < 0.1f)
      right = float3_normalize(float3_cross(axis, make_float3(1, 0, 0)));
      float3 forward = float3_normalize(float3_cross(axis, right));
      
      float3 v0 = float3_add(top_center,
                  float3_add(float3_scale(right, cos0 * top_radius),
                  float3_scale(forward, sin0 * top_radius)));
      float3 v1 = float3_add(top_center,
                  float3_add(float3_scale(right, cos1 * top_radius),
                  float3_scale(forward, sin1 * top_radius)));
      float3 v2 = float3_add(top_center,
                  float3_add(float3_scale(right, cos2 * top_radius),
                  float3_scale(forward, sin2 * top_radius)));
          
      // Reverse winding
      model->vertex_data[vertex_idx++] = (vertex_data_t){ v0, make_float2(0.5f + cos0 * 0.5f, 0.5f + sin0 * 0.5f), top_normal };
      model->vertex_data[vertex_idx++] = (vertex_data_t){ v2, make_float2(0.5f + cos2 * 0.5f, 0.5f + sin2 * 0.5f), top_normal };
      model->vertex_data[vertex_idx++] = (vertex_data_t){ v1, make_float2(0.5f + cos1 * 0.5f, 0.5f + sin1 * 0.5f), top_normal };
      
      model->face_normals[face_idx++] = top_normal;
    }
  }
      
  model->num_vertices = old_vertices + new_vertices;
  model->num_faces = old_faces + new_faces;
  
  return 0;
}

int generate_tree(model_t *model, float base_radius, float base_angle, float3 base_position, float branch_chance, usize level, const usize max_branches, const usize num_levels, const usize num_side_faces)  {
  static const float base_trunk_height = 8.0f;
  static const float spread_factor = 1.25f;
  float taper_factor = 0.85f;            // Minimal tapering for thick branches
  float upward_factor = 0.5f + (float)level * 0.15f;

  float growth_angle = base_angle;

  float segment_height, angle_offset;   // angle to aim branches
  float3 top_center;

  if (!model || num_side_faces < 3) {   // base case
    return -1;
  } else if (level == 0) {              // generate trunk
    segment_height = base_trunk_height;
    growth_angle += ((hash2((int)base_position.x, (int)base_position.z, g_world_config.seed)) * 0.15f); // add arandom offsett from rest of tree
    angle_offset = 0.1;                 // main trunk should have slight lean
    upward_factor = 1.0f;
  } else if (level == num_levels - 1) { // genrate top level
    segment_height = fmaxf(0.3f, ridgeNoise(base_position.x + level, base_position.y + level, g_world_config.seed) * 1.2f);
    taper_factor = 0.01;            // taper top branches to a point
    angle_offset = 0.0f;
  } else {                              // mid section
    segment_height = fmaxf(0.5f, ridgeNoise(base_position.x + level, base_position.y + level, g_world_config.seed) * 3.5f);
    angle_offset = spread_factor + (float)level * 0.05f;
  }

  float top_radius = base_radius * taper_factor;
  
  top_center = make_float3(
    base_position.x + sinf(growth_angle) * segment_height * angle_offset,
    base_position.y + segment_height * upward_factor,
    base_position.z + cosf(growth_angle) * segment_height * angle_offset
  );

  // generate our segment
  int ret = generate_tree_cylinder(model, base_radius, top_radius, segment_height, base_position, top_center, num_side_faces, base_angle, base_angle);

  if (level < num_levels - 1 && branch_chance > 0.2f) {
    // Add more variation to number of branches per level
    usize base_branches = (level == 0) ? 4 : 2;
    float branch_variation = hash2((int)(base_position.x * 37 + level), (int)(base_position.z * 41 + level), g_world_config.seed);
    usize extra_branches = (usize)(branch_variation * 2.0f + 0.5f); // 0-2 extra branches
    usize num_branches = base_branches + extra_branches;
    if (num_branches > max_branches) num_branches = max_branches;

    for (usize i = 0; i < num_branches; ++i) {
      float branch_growth_angle;

      // Should we generate this continuation?
      float branch_roll = hash2((int)(top_center.x * 13 + i * 17), (int)(top_center.z * 19 + level * 23), g_world_config.seed);

      // For trunk level (level 0), guarantee more branches by using higher effective chance
      float effective_branch_chance = (level == 0) ? fminf(0.95f, branch_chance + 0.15f) : branch_chance;
      if (branch_roll > effective_branch_chance) continue; // Skip this branch

      // More variation in branch thickness
      float radius_variation = hash2((int)(i * 29), (int)(level * 31), g_world_config.seed) * 0.3f; // Increased from 0.1f
      float growth_base_radius = top_radius * (0.75f + radius_variation); // Lowered base to allow thinner branches

      if (i == 0) { // main section - stay close to parent direction
        branch_growth_angle = growth_angle + (hash2((int)(top_center.x * 7), (int)(top_center.z * 11), g_world_config.seed)) * 0.5f;
      } else {      // branches - use more random distribution instead of even spacing
        // Sometimes use even distribution, sometimes completely random
        float distribution_mode = hash2((int)(top_center.x * 43 + level), (int)(top_center.z * 47 + i), g_world_config.seed);

        if (distribution_mode > 0.3f) {
          // Even distribution with variation (70% of the time)
          float base_radial_offset = ((float)(i - 1) / (float)(num_branches - 1)) * 2.0f * PI;
          float random_variation = hash2((int)(top_center.x * 11 + i), (int)(top_center.z * 13 + level), g_world_config.seed) * 0.8f;
          branch_growth_angle = base_radial_offset + random_variation;
        } else {
          // Completely random angle (30% of the time)
          branch_growth_angle = hash2((int)(top_center.x * 31 + i), (int)(top_center.z * 37 + level), g_world_config.seed) * 2.0f * PI;
        }
      }

      // Improved overlap system - start branches closer to parent end for better continuity
      float overlap_distance = growth_base_radius * 0.6f; // Increased overlap for better connection
      float branch_upward = (level == num_levels - 2) ? 0.3f : 0.3f + (float)(level + 1) * 0.15f; // Next level's upward factor

      // For better continuity, blend branch direction with parent direction - add more variation
      float base_parent_influence = (i == 0) ? 0.7f : 0.3f; // Main branch follows parent more closely
      float influence_variation = hash2((int)(top_center.x * 17 + i), (int)(top_center.z * 19 + level), g_world_config.seed) * 0.3f;
      float parent_influence = base_parent_influence + influence_variation;
      parent_influence = fmaxf(0.1f, fminf(0.9f, parent_influence)); // Clamp to reasonable range

      float parent_direction_x = sinf(growth_angle) * angle_offset;
      float parent_direction_z = cosf(growth_angle) * angle_offset;

      // Add some randomness to the spread factor per branch
      float branch_spread = spread_factor + (float)(level + 1) * 0.05f + hash2((int)(top_center.x * 23), (int)(top_center.z * 29), g_world_config.seed) * 0.3f;

      float3 branch_direction = make_float3(
        lerp(sinf(branch_growth_angle) * branch_spread, parent_direction_x, parent_influence),
        branch_upward,
        lerp(cosf(branch_growth_angle) * branch_spread, parent_direction_z, parent_influence)
      );

      branch_direction = float3_normalize(branch_direction);

      float3 branch_start = make_float3(
        top_center.x - branch_direction.x * overlap_distance,
        top_center.y - branch_direction.y * overlap_distance * 0.5f, // Reduce vertical overlap to avoid weird positioning
        top_center.z - branch_direction.z * overlap_distance
      );

      float branch_chance_decayed = branch_chance * 0.7;
      ret += generate_tree(model, growth_base_radius, branch_growth_angle, branch_start, branch_chance_decayed, level + 1, max_branches, num_levels, num_side_faces);
    }
  }

  return ret;
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
