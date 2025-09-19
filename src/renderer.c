#include "renderer.h"

#include <math.h>
#include <assert.h>
#include <float.h> // For FLT_MAX
#include <stdlib.h>
#include <SDL3/SDL.h>

#include "maths.h"

// Default fragment shader that just returns the input color
static inline u32 default_shader_func(u32 input_color, shader_context_t context, void *args, usize argc) {
  return input_color;
}

// Built-in default shader that just returns the input color
shader_t default_shader = { .func = default_shader_func, .argv = NULL, .argc = 0, .valid = true };

// Converts RGB components (0-255) to packed 32-bit RGBA8888 format, portablely using SDL
u32 rgb_to_888(u8 r, u8 g, u8 b) {
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);
  return SDL_MapRGBA(format, NULL, r, g, b, 255);
}


/**
* Calculates the signed area of a triangle defined by three points.
* The sign indicates the orientation of the triangle (positive for counter-clockwise, negative for clockwise).
* @param a The first vertex of the triangle.
* @param b The second vertex of the triangle.
* @param c The third vertex of the triangle.
* @return The signed area of the triangle.
*/
static inline f32 signed_triangle_area(const float2 a, const float2 b, const float2 c) {
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

/**
* Helper function to check if a point is inside a triangle using barycentric coordinates.
* @param a The first vertex of the triangle.
* @param b The second vertex of the triangle.
* @param c The third vertex of the triangle.
* @param p The point to check.
* @return True if the point is inside the triangle, false otherwise.
*/
static bool point_in_triangle(const float2 a, const float2 b, const float2 c, const float2 p, float3 *weights) {
  float area_abp = signed_triangle_area(a, b, p);
  float area_bcp = signed_triangle_area(b, c, p);
  float area_cap = signed_triangle_area(c, a, p);
  bool in_triangle = (area_abp >= 0 && area_bcp >= 0 && area_cap >= 0) ||
  (area_abp <= 0 && area_bcp <= 0 && area_cap <= 0);
  
  float area_sum = (area_abp + area_bcp + area_cap);
  area_sum = area_sum == 0 ? EPSILON : area_sum;

  float inv_area_sum = 1.0f / (area_sum);
  float weight_a = area_bcp * inv_area_sum;
  float weight_b = area_cap * inv_area_sum;
  float weight_c = area_abp * inv_area_sum;
  
  assert(weights != NULL);
  *weights = (float3){ weight_a, weight_b, weight_c };
  
  return in_triangle;
}

// Transforms a vector using the provided basis vectors
static inline float3 transform_vector(float3 ihat, float3 jhat, float3 khat, float3 vec) {
  return (float3){
    .x = vec.x * ihat.x + vec.y * jhat.x + vec.z * khat.x,
    .y = vec.x * ihat.y + vec.y * jhat.y + vec.z * khat.y,
    .z = vec.x * ihat.z + vec.y * jhat.z + vec.z * khat.z
  };
}

// Extracts the basis vectors (right, up, forward) from a transform's yaw and pitch
void transform_get_basis_vectors(transform_t *t, float3 *ihat, float3 *jhat, float3 *khat) {
  assert(t != NULL);
  assert(ihat != NULL);
  assert(jhat != NULL);
  assert(khat != NULL);
  
  // Apply yaw first, then pitch
  float cy = cosf(t->yaw);
  float sy = sinf(t->yaw);
  float cp = cosf(t->pitch);
  float sp = sinf(t->pitch);
  
  // Combined rotation matrix (yaw * pitch)
  *ihat = make_float3(cy, sy * sp, sy * cp);
  *jhat = make_float3(0, cp, -sp);
  *khat = make_float3(-sy, cy * sp, cy * cp);
}

// Extracts the inverse basis vectors (right, up, forward) from a transform's yaw and pitch
static void transform_get_inverse_basis_vectors(transform_t *t, float3 *ihat, float3 *jhat, float3 *khat) {
  assert(t != NULL);
  assert(ihat != NULL);
  assert(jhat != NULL);
  assert(khat != NULL);
  
  // For inverse, we transpose the rotation matrix (since rotation matrices are orthogonal)
  float cy = cosf(t->yaw);
  float sy = sinf(t->yaw);
  float cp = cosf(t->pitch);
  float sp = sinf(t->pitch);
  
  // Transposed matrix
  *ihat = make_float3(cy, 0, -sy);
  *jhat = make_float3(sy * sp, cp, cy * sp);
  *khat = make_float3(sy * cp, -sp, cy * cp);
}

// Transform a point from local space to world space using the transform's basis vectors and position
static float3 transform_to_world(transform_t *t, float3 p) {
  assert(t != NULL);
  
  float3 ihat, jhat, khat;
  transform_get_basis_vectors(t, &ihat, &jhat, &khat);
  
  // Transform the point and then translate by position
  float3 rotated = transform_vector(ihat, jhat, khat, p);
  return float3_add(rotated, t->position);
}

// Transform a point from world space to local space using the inverse of the transform's basis vectors and position
static float3 transform_to_local_point(transform_t *t, float3 p) {
  assert(t != NULL);
  
  float3 ihat, jhat, khat;
  transform_get_inverse_basis_vectors(t, &ihat, &jhat, &khat);
  
  // Translate first, then rotate
  float3 p_rel = float3_sub(p, t->position);
  return transform_vector(ihat, jhat, khat, p_rel);
}

// Apply a more subtle shading for specific triangle faces
void apply_side_shading(u32 *color, int tri) {
  assert(color != NULL);
  
  // Face order: bottom(10-11), top(8-9), front(0-1), back(2-3), right(6-7), left(4-5)
  if (tri == 2 || tri == 3 || tri == 4 || tri == 5 || tri == 10 || tri == 11) {
    // Extract RGB components
    uint8_t shade_r, shade_g, shade_b;
    SDL_GetRGB(*color, SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888), NULL, &shade_r, &shade_g, &shade_b);

    f32 brightness_factor = 0.75f;  // 25% darkening

    shade_r = (u8)((f32)shade_r * brightness_factor);
    shade_g = (u8)((f32)shade_g * brightness_factor);
    shade_b = (u8)((f32)shade_b * brightness_factor);

    // Repack into RGB888
    *color = rgb_to_888(shade_r, shade_g, shade_b);
  }
}

// Apply fog effect based on depth
static u32 apply_fog(u32 color, f32 depth) {
  f32 fog_factor = (depth - FOG_START) / (FOG_END - FOG_START);

  // Clamp fog factor
  if (fog_factor < 0.0f) fog_factor = 0.0f;
  if (fog_factor > 1.0f) fog_factor = 1.0f;

  // For full fog (fog_factor >= 0.99)
  if (fog_factor >= 0.99f) {
    return rgb_to_888(FOG_R, FOG_G, FOG_B);
  }

  uint8_t r, g, b;
  SDL_GetRGB(color, SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888), NULL, &r, &g, &b);

  // Interpolate between original color and fog color
  r = (uint8_t)(r * (1.0f - fog_factor) + FOG_R * fog_factor);
  g = (uint8_t)(g * (1.0f - fog_factor) + FOG_G * fog_factor);
  b = (uint8_t)(b * (1.0f - fog_factor) + FOG_B * fog_factor);
  return rgb_to_888(r, g, b);
}

void apply_fog_to_screen(game_state_t *state) {
  assert(state != NULL);

  for (int i = 0; i < WIN_WIDTH * WIN_HEIGHT; ++i) {
    if (state->depthbuffer[i] == FLT_MAX) continue; // Skip untouched pixels

    // Apply fog effect based on depth
    state->framebuffer[i] = apply_fog(state->framebuffer[i], state->depthbuffer[i]);
  }
}

// Initialize renderer state
void init_renderer(game_state_t *state) {
  assert(state != NULL);
  
  state->screen_dim = make_float2((f32)WIN_WIDTH, (f32)WIN_HEIGHT);
  state->frustum_bound = tanf(fov_over_2) * 1.4f;
  state->screen_height_world = tanf(fov_over_2) * 2;
  state->projection_scale = (float)WIN_WIDTH / state->screen_height_world;
  
  state->cam_right = make_float3(0, 0, 0);
  state->cam_up = make_float3(0, 0, 0);
  state->cam_forward = make_float3(0, 0, 0);
}

// Update camera basis vectors based on its current transform
void update_camera(game_state_t *state, transform_t *cam) {
  assert(state != NULL);
  assert(cam != NULL);
  
  transform_get_basis_vectors(cam, &state->cam_right, &state->cam_up, &state->cam_forward);
}

/**
* Renders a 3D model onto a 2D buffer using a basic rasterization pipeline.
* Applies transformations, projects vertices to screen space, performs simple frustum culling,
* back-face culling, and rasterizes triangles with depth testing.
*/
usize render_model(game_state_t *state, transform_t *cam, model_t *model) {
  assert(cam != NULL);
  assert(model != NULL);
  assert(model->vertices != NULL);
  assert(model->num_vertices % 3 == 0); // Ensure we have complete triangles
  assert(model->frag_shader != NULL);

  shader_t *frag_shader = model->frag_shader && model->frag_shader->valid ? model->frag_shader : &default_shader;
  assert(frag_shader->func != NULL);
  
  if (model->use_textures) {
    assert(state->texture_atlas != NULL);
    assert(model->num_uvs == model->num_vertices); // Ensure we have UVs
  }
  
  usize tris_rendered = 0;

  // Loop through each triangle in the model
  for (int tri = 0; tri < model->num_vertices / 3; ++tri) {
    // Transform vertices from model space to world space, then to view space
    float3 world_a = transform_to_world(&model->transform, model->vertices[tri * 3 + 0]);
    float3 world_b = transform_to_world(&model->transform, model->vertices[tri * 3 + 1]);
    float3 world_c = transform_to_world(&model->transform, model->vertices[tri * 3 + 2]);
    
    float3 view_a = transform_to_local_point(cam, world_a);
    float3 view_b = transform_to_local_point(cam, world_b);
    float3 view_c = transform_to_local_point(cam, world_c);
    
    // Basic frustum culling: skip triangle if all vertices are behind camera (z > 0 in view space)
    if (view_a.z > 0 && view_b.z > 0 && view_c.z > 0) continue;
    if (fmax(view_a.z, fmax(view_b.z, view_c.z)) > MAX_DEPTH)continue; // Cull if too far away
    
    // Additional frustum culling - check if triangle is completely outside view frustum
    bool outside_left   = (view_a.x < view_a.z * state->frustum_bound && view_b.x < view_b.z * state->frustum_bound && view_c.x < view_c.z * state->frustum_bound);
    bool outside_right  = (view_a.x > -view_a.z * state->frustum_bound && view_b.x > -view_b.z * state->frustum_bound && view_c.x > -view_c.z * state->frustum_bound);
    bool outside_top    = (view_a.y > -view_a.z * state->frustum_bound && view_b.y > -view_b.z * state->frustum_bound && view_c.y > -view_c.z * state->frustum_bound);
    bool outside_bottom = (view_a.y < view_a.z * state->frustum_bound && view_b.y < view_b.z * state->frustum_bound && view_c.y < view_c.z * state->frustum_bound);

    if (outside_left || outside_right || outside_top || outside_bottom) continue;
    
    // Use pre-computed face normal for back-face culling
    float3 model_normal = model->face_normals[tri];
    
    // Transform face normal from model space to world space (rotation only)
    float3 ihat, jhat, khat;
    transform_get_basis_vectors(&model->transform, &ihat, &jhat, &khat);
    float3 triangle_normal = transform_vector(ihat, jhat, khat, model_normal);
    
    // Vector from camera to triangle center
    float3 triangle_center = float3_scale(float3_add(float3_add(world_a, world_b), world_c), 1.0f/3.0f);
    float3 view_direction = float3_normalize(float3_sub(triangle_center, cam->position));  // Point from camera to triangle
    
    // Check if triangle is facing toward camera
    float dot_product = float3_dot(triangle_normal, view_direction);
    if (dot_product <= 0) continue; // Triangle is facing away from camera
    
    // Use pre-computed projection constants
    float pixels_per_world_unit_a = state->projection_scale / view_a.z;
    float pixels_per_world_unit_b = state->projection_scale / view_b.z;
    float pixels_per_world_unit_c = state->projection_scale / view_c.z;
    
    float2 pixel_offset_a = float2_scale(make_float2(view_a.x, view_a.y), pixels_per_world_unit_a);
    float2 pixel_offset_b = float2_scale(make_float2(view_b.x, view_b.y), pixels_per_world_unit_b);
    float2 pixel_offset_c = float2_scale(make_float2(view_c.x, view_c.y), pixels_per_world_unit_c);
    
    float2 screen_a = float2_add(float2_scale(state->screen_dim, 0.5f), pixel_offset_a);
    float2 screen_b = float2_add(float2_scale(state->screen_dim, 0.5f), pixel_offset_b);
    float2 screen_c = float2_add(float2_scale(state->screen_dim, 0.5f), pixel_offset_c);
    
    // triangle points in screen space, with their associated depth
    float3 a = make_float3(screen_a.x, screen_a.y, view_a.z);
    float3 b = make_float3(screen_b.x, screen_b.y, view_b.z);
    float3 c = make_float3(screen_c.x, screen_c.y, view_c.z);
    
    // Compute triangle bounding box (clamped to screen boundaries)
    float min_x = fmaxf(0.0f, floorf(fminf(a.x, fminf(b.x, c.x))));
    float max_x = fminf(WIN_WIDTH - 1, ceilf(fmaxf(a.x, fmaxf(b.x, c.x))));
    float min_y = fmaxf(0.0f, floorf(fminf(a.y, fminf(b.y, c.y))));
    float max_y = fminf(WIN_HEIGHT - 1, ceilf(fmaxf(a.y, fmaxf(b.y, c.y))));

    // Get the UV coordinates for the current triangle's vertices
    float2 uv_a = model->uvs[tri * 3 + 0];
    float2 uv_b = model->uvs[tri * 3 + 1];
    float2 uv_c = model->uvs[tri * 3 + 2];

    // Perspective-correct UVs: Pre-divide UVs by their respective 1/w
    float2 uv_a_prime = float2_divide(uv_a, a.z);
    float2 uv_b_prime = float2_divide(uv_b, b.z);
    float2 uv_c_prime = float2_divide(uv_c, c.z);

    // Precompute color for this triangle (use different colors for debugging)
    uint32_t flat_color = rgb_to_888(255, 0, 255); // Magenta for all triangles
    
    // Rasterize only within the computed bounding box
    for (int y = (int)min_y; y <= (int)max_y; ++y) {
      int pixel_base = y * WIN_WIDTH; // Precompute row offset
      
      for (int x = (int)min_x; x <= (int)max_x; ++x) {
        float2 p = { (float)x, (float)y }; // Current pixel coordinate
        
        float3 weights; // Barycentric coordinates
        // Check if the current pixel is inside the triangle
        if (point_in_triangle(make_float2(a.x, a.y), make_float2(b.x, b.y), make_float2(c.x, c.y), p, &weights)) {
          // Interpolate depth using barycentric coordinates
          float new_depth = -1.0f / (weights.x / a.z + weights.y / b.z + weights.z / c.z);
          
          // Z-buffering: check if this pixel is closer than what's already drawn at this position
          int pixel_idx = pixel_base + x; // Use precomputed row offset
          if (new_depth < state->depthbuffer[pixel_idx]) {
            uint32_t output_color;
            
            if (model->use_textures && state->texture_atlas != NULL) {
              // Interpolate perspective-corrected UVs
              float interpolated_u_prime = weights.x * uv_a_prime.x + weights.y * uv_b_prime.x + weights.z * uv_c_prime.x;
              float interpolated_v_prime = weights.x * uv_a_prime.y + weights.y * uv_b_prime.y + weights.z * uv_c_prime.y;

              // Divide by interpolated 1/w to get correct perspective UVs
              float final_u = interpolated_u_prime * -new_depth;
              float final_v = interpolated_v_prime * -new_depth;

              // Map normalized UVs [0.0, 1.0] to texture pixel coordinates (optimized)
              int tex_x = (int)(final_u * (f32)ATLAS_WIDTH_PX);
              int tex_y = (int)(final_v * (f32)ATLAS_HEIGHT_PX);

              // Fast clamp using bit operations and conditionals
              tex_x = (tex_x < 0) ? 0 : ((tex_x > ATLAS_WIDTH_PX - 1) ? ATLAS_WIDTH_PX - 1 : tex_x);
              tex_y = (tex_y < 0) ? 0 : ((tex_y > ATLAS_HEIGHT_PX - 1) ? ATLAS_HEIGHT_PX - 1 : tex_y);

              // Skip Magenta pixels, transparency support
              if (state->texture_atlas[tex_y * ATLAS_WIDTH_PX + tex_x] == 0xFF00FF) continue;

              output_color = state->texture_atlas[tex_y * ATLAS_WIDTH_PX + tex_x];
            } else {
              output_color = flat_color; // Use flat color if no texture
            }

            // Populate shader context
            shader_context_t shader_ctx = {0};

            // Interpolate world position using barycentric coordinates
            shader_ctx.world_pos = float3_add(
              float3_add(
                float3_scale(world_a, weights.x),
                float3_scale(world_b, weights.y)
              ),
              float3_scale(world_c, weights.z)
            );

            // Screen position
            shader_ctx.screen_pos = make_float2((float)x, (float)y);

            // UV coordinates (interpolated if available)
            if (model->use_textures && model->uvs != NULL) {
              float2 uv_a = model->uvs[tri * 3 + 0];
              float2 uv_b = model->uvs[tri * 3 + 1];
              float2 uv_c = model->uvs[tri * 3 + 2];

              shader_ctx.uv = make_float2(
                weights.x * uv_a.x + weights.y * uv_b.x + weights.z * uv_c.x,
                weights.x * uv_a.y + weights.y * uv_b.y + weights.z * uv_c.y
              );
            } else {
              shader_ctx.uv = make_float2(0.0f, 0.0f);
            }

            // Depth
            shader_ctx.depth = new_depth;

            // Face normal (already transformed to world space)
            shader_ctx.normal = triangle_normal;

            // View direction (from fragment to camera)
            shader_ctx.view_dir = float3_normalize(float3_sub(cam->position, shader_ctx.world_pos));

            // Time
            shader_ctx.time = SDL_GetTicks() / 1000.0f;

            output_color = frag_shader->func(output_color, shader_ctx, frag_shader->argv, frag_shader->argc);

            apply_side_shading(&output_color, tri);
            
            state->framebuffer[pixel_idx] = output_color; // Draw the pixel
            state->depthbuffer[pixel_idx] = new_depth; // Update depth buffer
          }

        }
      }
    }

    tris_rendered++;
  }

  return tris_rendered;
}

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
  
  model->vertices = malloc(total_triangle_vertices * sizeof(float3));
  model->uvs = malloc(total_triangle_vertices * sizeof(float2));
  model->face_normals = malloc(total_triangles * sizeof(float3));
  if (!model->vertices || !model->uvs || !model->face_normals) {
    free(model->vertices); 
    free(model->uvs);
    free(model->face_normals); 
    return -1;
  }
  
  // Create temporary grid of vertices
  float3 *grid_verts = malloc(grid_vertices * sizeof(float3));
  float2 *grid_uvs = malloc(grid_vertices * sizeof(float2));
  if (!grid_verts || !grid_uvs) {
    free(model->vertices);
    free(model->uvs);
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
      model->vertices[vertex_idx] = grid_verts[tl];
      model->uvs[vertex_idx] = grid_uvs[tl];
      vertex_idx++;
      
      model->vertices[vertex_idx] = grid_verts[bl];
      model->uvs[vertex_idx] = grid_uvs[bl];
      vertex_idx++;
      
      model->vertices[vertex_idx] = grid_verts[tr];
      model->uvs[vertex_idx] = grid_uvs[tr];
      vertex_idx++;
      
      // Second triangle: TR -> BL -> BR (CCW)
      model->vertices[vertex_idx] = grid_verts[tr];
      model->uvs[vertex_idx] = grid_uvs[tr];
      vertex_idx++;
      
      model->vertices[vertex_idx] = grid_verts[bl];
      model->uvs[vertex_idx] = grid_uvs[bl];
      vertex_idx++;
      
      model->vertices[vertex_idx] = grid_verts[br];
      model->uvs[vertex_idx] = grid_uvs[br];
      vertex_idx++;
    }
  }
  
  model->num_vertices = total_triangle_vertices;
  model->num_uvs = total_triangle_vertices;
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
  
  // Allocate memory for vertices, UVs, and face normals
  model->vertices = malloc(CUBE_VERTS * sizeof(float3));
  model->uvs = malloc(CUBE_VERTS * sizeof(float2));
  model->face_normals = malloc((CUBE_VERTS / 3) * sizeof(float3));
  
  if (!model->vertices || !model->uvs || !model->face_normals) {
    free(model->vertices);
    free(model->uvs);
    free(model->face_normals);
    return -1;
  }

  // Half extents for more intuitive vertex positioning
  float3 half = {size.x * 0.5f, size.y * 0.5f, size.z * 0.5f};
  
  // Generate vertices for each face - maintain counter-clockwise winding order
  int v = 0;  // vertex index
  
  // Front face (-Z, closest to camera)
  model->vertices[v++] = (float3){-half.x, -half.y, -half.z};
  model->vertices[v++] = (float3){ half.x, -half.y, -half.z};
  model->vertices[v++] = (float3){ half.x,  half.y, -half.z};
  model->vertices[v++] = (float3){-half.x, -half.y, -half.z};
  model->vertices[v++] = (float3){ half.x,  half.y, -half.z};
  model->vertices[v++] = (float3){-half.x,  half.y, -half.z};
  
  // Back face (+Z, furthest from camera)
  model->vertices[v++] = (float3){ half.x, -half.y,  half.z};
  model->vertices[v++] = (float3){-half.x, -half.y,  half.z};
  model->vertices[v++] = (float3){-half.x,  half.y,  half.z};
  model->vertices[v++] = (float3){ half.x, -half.y,  half.z};
  model->vertices[v++] = (float3){-half.x,  half.y,  half.z};
  model->vertices[v++] = (float3){ half.x,  half.y,  half.z};
  
  // Left face (+X)
  model->vertices[v++] = (float3){ half.x, -half.y, -half.z};
  model->vertices[v++] = (float3){ half.x, -half.y,  half.z};
  model->vertices[v++] = (float3){ half.x,  half.y,  half.z};
  model->vertices[v++] = (float3){ half.x, -half.y, -half.z};
  model->vertices[v++] = (float3){ half.x,  half.y,  half.z};
  model->vertices[v++] = (float3){ half.x,  half.y, -half.z};
  
  // Right face (-X)
  model->vertices[v++] = (float3){-half.x, -half.y,  half.z};
  model->vertices[v++] = (float3){-half.x, -half.y, -half.z};
  model->vertices[v++] = (float3){-half.x,  half.y, -half.z};
  model->vertices[v++] = (float3){-half.x, -half.y,  half.z};
  model->vertices[v++] = (float3){-half.x,  half.y, -half.z};
  model->vertices[v++] = (float3){-half.x,  half.y,  half.z};
  
  // Top face (+Y)
  model->vertices[v++] = (float3){-half.x,  half.y, -half.z};
  model->vertices[v++] = (float3){ half.x,  half.y, -half.z};
  model->vertices[v++] = (float3){ half.x,  half.y,  half.z};
  model->vertices[v++] = (float3){-half.x,  half.y, -half.z};
  model->vertices[v++] = (float3){ half.x,  half.y,  half.z};
  model->vertices[v++] = (float3){-half.x,  half.y,  half.z};
  
  // Bottom face (-Y)
  model->vertices[v++] = (float3){-half.x, -half.y,  half.z};
  model->vertices[v++] = (float3){ half.x, -half.y,  half.z};
  model->vertices[v++] = (float3){ half.x, -half.y, -half.z};
  model->vertices[v++] = (float3){-half.x, -half.y,  half.z};
  model->vertices[v++] = (float3){ half.x, -half.y, -half.z};
  model->vertices[v++] = (float3){-half.x, -half.y, -half.z};

  // Generate UVs for each vertex
  for (int i = 0; i < CUBE_VERTS; i += 6) {
    // First triangle
    model->uvs[i + 0] = (float2){0.0f, 0.0f};
    model->uvs[i + 1] = (float2){1.0f, 0.0f};
    model->uvs[i + 2] = (float2){1.0f, 1.0f};
    // Second triangle
    model->uvs[i + 3] = (float2){0.0f, 0.0f};
    model->uvs[i + 4] = (float2){1.0f, 1.0f};
    model->uvs[i + 5] = (float2){0.0f, 1.0f};
  }
  
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
  model->num_uvs = CUBE_VERTS;
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
  
  // Allocate memory for vertices, UVs, and face normals
  model->vertices = (float3*)malloc(num_triangles * 3 * sizeof(float3)); // 3 vertices per triangle
  model->uvs = (float2*)malloc(num_triangles * 3 * sizeof(float2));      // 3 UVs per triangle
  model->face_normals = (float3*)malloc(num_triangles * sizeof(float3)); // 1 normal per triangle
  
  if (!model->vertices || !model->uvs || !model->face_normals) {
    if (model->vertices) free(model->vertices);
    if (model->uvs) free(model->uvs);
    if (model->face_normals) free(model->face_normals);
    return -1;
  }
  
  // Generate temporary vertex and UV grids
  float3* temp_vertices = (float3*)malloc(num_vertices * sizeof(float3));
  float2* temp_uvs = (float2*)malloc(num_vertices * sizeof(float2));
  
  if (!temp_vertices || !temp_uvs) {
    free(model->vertices);
    free(model->uvs);
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
      
      model->vertices[vertex_index]     = v0;
      model->vertices[vertex_index + 1] = v1;
      model->vertices[vertex_index + 2] = v2;
      
      model->uvs[vertex_index]     = temp_uvs[current];
      model->uvs[vertex_index + 1] = temp_uvs[next];
      model->uvs[vertex_index + 2] = temp_uvs[current + 1];
      
      float3 edge1 = float3_sub(v1, v0);
      float3 edge2 = float3_sub(v2, v0);
      model->face_normals[face_index] = float3_normalize(float3_cross(edge1, edge2));
      face_index++;
      
      // --- Second triangle of quad ---
      float3 v3 = temp_vertices[next];
      float3 v4 = temp_vertices[next + 1];
      float3 v5 = temp_vertices[current + 1];
      
      model->vertices[vertex_index + 3] = v1;
      model->vertices[vertex_index + 4] = v4;
      model->vertices[vertex_index + 5] = v2;
      
      model->uvs[vertex_index + 3] = temp_uvs[next];
      model->uvs[vertex_index + 4] = temp_uvs[next + 1];
      model->uvs[vertex_index + 5] = temp_uvs[current + 1];

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
  model->num_uvs = num_triangles * 3;
  model->num_faces = num_triangles;
  
  model->transform.position = position;
  model->transform.yaw = 0.0f;
  model->transform.pitch = 0.0f;
  model->scale = make_float3(1.0f, 1.0f, 1.0f);

  return 0;
}

shader_t make_shader(shader_func func, void *argv, usize argc) {
  return (shader_t) {
    .func = func,
    .argv = argv,
    .argc = argc,
    .valid = true
  };
}