#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L // enable precision timing
#endif

#include <shader-works/renderer.h>

#include <math.h>
#include <assert.h>
#include <float.h> // For FLT_MAX
#include <stdlib.h>
#include <time.h>

#include <shader-works/maths.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef SHADER_WORKS_USE_PTHREADS
#include <pthread.h>
#include <unistd.h> // For sysconf

bool render_triangle(triangle_context_t *ctx);

typedef struct {
  triangle_context_t base_ctx;
  int total_triangles;
  int* next_triangle;
  usize* triangles_rendered;
} worker_thread_data_t;

static void* worker_thread(void* arg) {
  worker_thread_data_t* data = (worker_thread_data_t*)arg;
  usize local_count = 0;
  int tri;

  // Atomically grab triangles one at a time
  while ((tri = __sync_fetch_and_add(data->next_triangle, 1)) < data->total_triangles) {
    data->base_ctx.tri = tri;
    if (render_triangle(&data->base_ctx)) {
      local_count++;
    }
  }

  *data->triangles_rendered = local_count;
  return NULL;
}
#endif

// Calculates the signed area of a triangle defined by three points.
static inline f32 signed_triangle_area(const float2 a, const float2 b, const float2 c) {
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

// Integer version of signed triangle area calculation (much faster)
static inline int32_t signed_triangle_area_int(int32_t ax, int32_t ay, int32_t bx, int32_t by, int32_t cx, int32_t cy) {
  return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
}

// Helper function to check if a point is inside a triangle using barycentric coordinates.
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

// Integer version of point_in_triangle for faster rasterization
static bool point_in_triangle_int(int32_t ax, int32_t ay, int32_t bx, int32_t by, int32_t cx, int32_t cy, int32_t px, int32_t py, float3 *weights) {
  int32_t area_abp = signed_triangle_area_int(ax, ay, bx, by, px, py);
  int32_t area_bcp = signed_triangle_area_int(bx, by, cx, cy, px, py);
  int32_t area_cap = signed_triangle_area_int(cx, cy, ax, ay, px, py);

  // Check if point is inside triangle (all same sign)
  bool in_triangle = (area_abp >= 0 && area_bcp >= 0 && area_cap >= 0) ||
                     (area_abp <= 0 && area_bcp <= 0 && area_cap <= 0);

  // Calculate barycentric coordinates using floating point for final weights
  int32_t area_sum = area_abp + area_bcp + area_cap;

  // Handle degenerate case (avoid division by zero)
  if (area_sum == 0) {
    *weights = (float3){1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f}; // Equal weights
    return false;
  }

  // Calculate weights using floating point division (only at the end)
  float inv_area_sum = 1.0f / (float)area_sum;
  float weight_a = (float)area_bcp * inv_area_sum;
  float weight_b = (float)area_cap * inv_area_sum;
  float weight_c = (float)area_abp * inv_area_sum;

  assert(weights != NULL);
  *weights = (float3){weight_a, weight_b, weight_c};

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
static void transform_get_basis_vectors(transform_t *t, float3 *ihat, float3 *jhat, float3 *khat) {
  assert(t != NULL);
  assert(ihat != NULL);
  assert(jhat != NULL);
  assert(khat != NULL);

  // Apply pitch first, then yaw (standard FPS rotation order)
  float cy = cosf(t->yaw);
  float sy = sinf(t->yaw);
  float cp = cosf(t->pitch);
  float sp = sinf(t->pitch);

  // Combined rotation matrix (pitch * yaw) - standard FPS order
  *ihat = make_float3(cy, 0, -sy);           // Right vector (no coupling)
  *jhat = make_float3(sy * sp, cp, cy * sp); // Up vector
  *khat = make_float3(sy * cp, -sp, cy * cp); // Forward vector
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

  // Transposed matrix (transpose of the new FPS matrix)
  *ihat = make_float3(cy, sy * sp, sy * cp);      // Transpose of right vector
  *jhat = make_float3(0, cp, -sp);                // Transpose of up vector
  *khat = make_float3(-sy, cy * sp, cy * cp);     // Transpose of forward vector
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

// Apply the vertex shader to a triangle's vertices
static void apply_vertex_shader(model_t *model, vertex_shader_t *shader, vertex_context_t *context, usize tri, float3 *out_a, float3 *out_b, float3 *out_c) {
  assert(model != NULL);
  assert(shader != NULL && shader->valid);
  assert(context != NULL);
  assert(out_a != NULL && out_b != NULL && out_c != NULL);

  // Cache-friendly: single memory fetch per vertex gets all data
  vertex_data_t *v0 = &model->vertex_data[tri * 3 + 0];
  vertex_data_t *v1 = &model->vertex_data[tri * 3 + 1];
  vertex_data_t *v2 = &model->vertex_data[tri * 3 + 2];

  context->vertex_index = 0;
  context->original_vertex = v0->position;
  context->original_uv = model->use_textures ? v0->uv : make_float2(0, 0);
  context->original_normal = model->use_textures ? &model->face_normals[tri] : NULL;
  context->triangle_index = tri;
  *out_a = shader->func(context, shader->argv, shader->argc);

  context->vertex_index = 1;
  context->original_vertex = v1->position;
  context->original_uv = model->use_textures ? v1->uv : make_float2(0, 0);
  context->original_normal = model->use_textures ? &model->face_normals[tri] : NULL;
  *out_b = shader->func(context, shader->argv, shader->argc);

  context->vertex_index = 2;
  context->original_vertex = v2->position;
  context->original_uv = model->use_textures ? v2->uv : make_float2(0, 0);
  context->original_normal = model->use_textures ? &model->face_normals[tri] : NULL;
  *out_c = shader->func(context, shader->argv, shader->argc);
}

// Basic frustum culling: returns true if triangle is completely outside the frustum
static bool frustum_cull_triangle(float3 a, float3 b, float3 c, f32 frustum_bound, f32 max_depth) {
 // Basic frustum culling: skip triangle if all vertices are behind camera (z > 0 in view space)
  if (a.z > 0 && b.z > 0 && c.z > 0) return true;
  if (fmax(a.z, fmax(b.z, c.z)) > max_depth) return true; // Cull if too far away

  // Additional frustum culling - check if triangle is completely outside view frustum
  bool outside_left   = (a.x < a.z * frustum_bound && b.x < b.z * frustum_bound && c.x < c.z * frustum_bound);
  bool outside_right  = (a.x > -a.z * frustum_bound && b.x > -b.z * frustum_bound && c.x > -c.z * frustum_bound);
  bool outside_top    = (a.y > -a.z * frustum_bound && b.y > -b.z * frustum_bound && c.y > -c.z * frustum_bound);
  bool outside_bottom = (a.y < a.z * frustum_bound && b.y < b.z * frustum_bound && c.y < c.z * frustum_bound);

  return outside_left || outside_right || outside_top || outside_bottom;
}

// Apply fog effect based on depth
static u32 apply_fog(u32 color, f32 depth, f32 fog_start, f32 fog_end, u8 fog_r, u8 fog_g, u8 fog_b) {
  f32 fog_factor = (depth - fog_start) / (fog_end - fog_start);

  // Clamp fog factor
  if (fog_factor < 0.0f) fog_factor = 0.0f;
  if (fog_factor > 1.0f) fog_factor = 1.0f;

  // For full fog (fog_factor >= 0.99)
  if (fog_factor >= 0.99f) {
    return rgb_to_u32(fog_r, fog_g, fog_b);
  }

  uint8_t r, g, b;
  u32_to_rgb(color, &r, &g, &b);

  // Interpolate between original color and fog color
  r = (uint8_t)(r * (1.0f - fog_factor) + fog_r * fog_factor);
  g = (uint8_t)(g * (1.0f - fog_factor) + fog_g * fog_factor);
  b = (uint8_t)(b * (1.0f - fog_factor) + fog_b * fog_factor);
  return rgb_to_u32(r, g, b);
}

void apply_fog_to_screen(renderer_t *state, f32 fog_start, f32 fog_end, u8 fog_r, u8 fog_g, u8 fog_b) {
  assert(state != NULL);

  int total_pixels = (int)(state->screen_dim.x * state->screen_dim.y);
  for (int i = 0; i < total_pixels; ++i) {
    if (state->depthbuffer[i] == FLT_MAX) continue; // Skip untouched pixels

    // Apply fog effect based on depth
    state->framebuffer[i] = apply_fog(state->framebuffer[i], state->depthbuffer[i], fog_start, fog_end, fog_r, fog_g, fog_b);
  }
}

// Initialize renderer state
void init_renderer(renderer_t *state, u32 width, u32 height, u32 atlas_width, u32 atlas_height, u32 *framebuffer, f32 *depthbuffer, f32 max_depth) {
  assert(state != NULL);
  assert(framebuffer != NULL);
  assert(depthbuffer != NULL);

  state->framebuffer = framebuffer;
  state->depthbuffer = depthbuffer;
  state->screen_dim = make_float2((f32)width, (f32)height);
  state->atlas_dim = make_float2((f32)atlas_width, (f32)atlas_height);
  state->frustum_bound = tanf(fov_over_2) * 1.4f;
  state->screen_height_world = tanf(fov_over_2) * 2;
  state->projection_scale = (float)width / state->screen_height_world;
  state->max_depth = max_depth;

  state->time = 0;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  state->start_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000; // milliseconds
  state->wireframe_mode = false;
  state->texture_atlas = NULL;

  state->cam_right = make_float3(0, 0, 0);
  state->cam_up = make_float3(0, 0, 0);
  state->cam_forward = make_float3(0, 0, 0);
}

// Update camera basis vectors based on its current transform
void update_camera(renderer_t *state, transform_t *cam) {
  assert(state != NULL);
  assert(cam != NULL);

  transform_get_basis_vectors(cam, &state->cam_right, &state->cam_up, &state->cam_forward);
}

bool render_triangle(triangle_context_t *ctx) {
  // Call vertex shader to get transformed vertices
  float3 transformed_a, transformed_b, transformed_c;
  apply_vertex_shader(ctx->model, ctx->vertex_shader, &ctx->vertex_ctx, ctx->tri, &transformed_a, &transformed_b, &transformed_c);

  // Transform vertices from model space to world space, then to view space
  float3 world_a = transform_to_world(&ctx->model->transform, transformed_a);
  float3 world_b = transform_to_world(&ctx->model->transform, transformed_b);
  float3 world_c = transform_to_world(&ctx->model->transform, transformed_c);

  float3 view_a = transform_to_local_point(ctx->cam, world_a);
  float3 view_b = transform_to_local_point(ctx->cam, world_b);
  float3 view_c = transform_to_local_point(ctx->cam, world_c);

  if (frustum_cull_triangle(view_a, view_b, view_c, ctx->frustum_bound, ctx->max_depth))
    return false; // Triangle is outside the view frustum

  // Use pre-computed face normal for back-face culling
  float3 model_normal = ctx->model->face_normals[ctx->tri];

  // Transform face normal from model space to world space (rotation only)
  float3 ihat, jhat, khat;
  transform_get_basis_vectors(&ctx->model->transform, &ihat, &jhat, &khat);
  float3 triangle_normal = transform_vector(ihat, jhat, khat, model_normal);

  // Vector from camera to triangle center
  float3 triangle_center = float3_scale(float3_add(float3_add(world_a, world_b), world_c), 1.0f/3.0f);
  float3 view_direction = float3_normalize(float3_sub(triangle_center, ctx->cam->position));  // Point from camera to triangle

  // Check if triangle is facing toward camera
  float dot_product = float3_dot(triangle_normal, view_direction);
  if (dot_product <= EPSILON) return false; // Triangle is facing away from camera

  // Use pre-computed projection constants
  float pixels_per_world_unit_a = ctx->state->projection_scale / view_a.z;
  float pixels_per_world_unit_b = ctx->state->projection_scale / view_b.z;
  float pixels_per_world_unit_c = ctx->state->projection_scale / view_c.z;

  float2 pixel_offset_a = float2_scale(make_float2(view_a.x, view_a.y), pixels_per_world_unit_a);
  float2 pixel_offset_b = float2_scale(make_float2(view_b.x, view_b.y), pixels_per_world_unit_b);
  float2 pixel_offset_c = float2_scale(make_float2(view_c.x, view_c.y), pixels_per_world_unit_c);

  float2 screen_a = float2_add(float2_scale(ctx->state->screen_dim, 0.5f), pixel_offset_a);
  float2 screen_b = float2_add(float2_scale(ctx->state->screen_dim, 0.5f), pixel_offset_b);
  float2 screen_c = float2_add(float2_scale(ctx->state->screen_dim, 0.5f), pixel_offset_c);

  // triangle points in screen space, with their associated depth
  float3 a = make_float3(screen_a.x, screen_a.y, view_a.z);
  float3 b = make_float3(screen_b.x, screen_b.y, view_b.z);
  float3 c = make_float3(screen_c.x, screen_c.y, view_c.z);

  // Compute triangle bounding box (clamped to screen boundaries)
  float min_x = fmaxf(0.0f, floorf(fminf(a.x, fminf(b.x, c.x))));
  float max_x = fminf(ctx->state->screen_dim.x - 1, ceilf(fmaxf(a.x, fmaxf(b.x, c.x))));
  float min_y = fmaxf(0.0f, floorf(fminf(a.y, fminf(b.y, c.y))));
  float max_y = fminf(ctx->state->screen_dim.y - 1, ceilf(fmaxf(a.y, fmaxf(b.y, c.y))));

  // Get the UV coordinates for the current triangle's vertices (cache-friendly access)
  float2 uv_a = ctx->model->vertex_data[ctx->tri * 3 + 0].uv;
  float2 uv_b = ctx->model->vertex_data[ctx->tri * 3 + 1].uv;
  float2 uv_c = ctx->model->vertex_data[ctx->tri * 3 + 2].uv;

  // Perspective-correct UVs: Pre-divide UVs by their respective 1/w (with epsilon to prevent divide by zero)
  float safe_a_z = (fabsf(a.z) < EPSILON) ? (a.z < 0 ? -EPSILON : EPSILON) : a.z;
  float safe_b_z = (fabsf(b.z) < EPSILON) ? (b.z < 0 ? -EPSILON : EPSILON) : b.z;
  float safe_c_z = (fabsf(c.z) < EPSILON) ? (c.z < 0 ? -EPSILON : EPSILON) : c.z;

  float2 uv_a_prime = float2_divide(uv_a, safe_a_z);
  float2 uv_b_prime = float2_divide(uv_b, safe_b_z);
  float2 uv_c_prime = float2_divide(uv_c, safe_c_z);

  // Precompute color for this triangle (use different colors for debugging)
  uint32_t flat_color = rgb_to_u32(255, 10, 255); // Magenta for all triangles

   ctx->frag_ctx.normal = triangle_normal;

  // Convert screen coordinates to integers for faster triangle tests
  int32_t a_x = (int32_t)(a.x * 256.0f); // Use 8.24 fixed point for subpixel precision
  int32_t a_y = (int32_t)(a.y * 256.0f);
  int32_t b_x = (int32_t)(b.x * 256.0f);
  int32_t b_y = (int32_t)(b.y * 256.0f);
  int32_t c_x = (int32_t)(c.x * 256.0f);
  int32_t c_y = (int32_t)(c.y * 256.0f);

  // Rasterize only within the computed bounding box
  for (int y = (int)min_y; y <= (int)max_y; ++y) {
    int pixel_base = y * (int)ctx->state->screen_dim.x; // Precompute row offset

    for (int x = (int)min_x; x <= (int)max_x; ++x) {
      // Use integer coordinates for pixel (with subpixel precision)
      int32_t p_x = x * 256; // Convert to same 8.24 format
      int32_t p_y = y * 256;

      float3 weights; // Barycentric coordinates
      // Check if the current pixel is inside the triangle using integer math
      if (point_in_triangle_int(a_x, a_y, b_x, b_y, c_x, c_y, p_x, p_y, &weights)) {
        // Interpolate depth using barycentric coordinates (with safe Z values)
        float new_depth = -1.0f / (weights.x / safe_a_z + weights.y / safe_b_z + weights.z / safe_c_z);

        // Z-buffering: check if this pixel is closer than what's already drawn at this position
        int pixel_idx = pixel_base + x; // Use precomputed row offset
        if (new_depth < ctx->state->depthbuffer[pixel_idx]) {
          uint32_t output_color;

          if (ctx->state->wireframe_mode) {
            // Always update depth buffer for entire triangle to block back faces
            ctx->state->depthbuffer[pixel_idx] = new_depth;

            // Only draw visible pixels at triangle edges
            if (weights.x < 0.02f || weights.y < 0.02f || weights.z < 0.02f) {
              output_color = rgb_to_u32(0, 0, 0); // Black for wireframe edges
              ctx->state->framebuffer[pixel_idx] = output_color; // Draw the edge pixel
            }
            continue;
          }

          // Normal shaded rendering path
          if (ctx->model->use_textures && ctx->state->texture_atlas != NULL) {
            // Interpolate perspective-corrected UVs using floating point (simpler and faster)
            float interpolated_u_prime = weights.x * uv_a_prime.x + weights.y * uv_b_prime.x + weights.z * uv_c_prime.x;
            float interpolated_v_prime = weights.x * uv_a_prime.y + weights.y * uv_b_prime.y + weights.z * uv_c_prime.y;

            // Divide by interpolated 1/w to get correct perspective UVs
            float final_u = interpolated_u_prime * -new_depth;
            float final_v = interpolated_v_prime * -new_depth;

            // Map normalized UVs [0.0, 1.0] to texture pixel coordinates (optimized)
            int tex_x = (int)(final_u * (f32)ctx->state->atlas_dim.x);
            int tex_y = (int)(final_v * (f32)ctx->state->atlas_dim.y);

            // Fast clamp using bit operations and conditionals
            tex_x = (tex_x < 0) ? 0 : ((tex_x > ctx->state->atlas_dim.x - 1) ? ctx->state->atlas_dim.x - 1 : tex_x);
            tex_y = (tex_y < 0) ? 0 : ((tex_y > ctx->state->atlas_dim.y - 1) ? ctx->state->atlas_dim.y - 1 : tex_y);

            output_color = ctx->state->texture_atlas[tex_y * (int)ctx->state->atlas_dim.x + tex_x];
          } else {
            output_color = flat_color; // Use flat color if no texture
          }

          // Interpolate world position using barycentric coordinates
          ctx->frag_ctx.world_pos = float3_add(
            float3_add(
              float3_scale(world_a, weights.x),
              float3_scale(world_b, weights.y)
            ),
            float3_scale(world_c, weights.z)
          );

          // Screen position
          ctx->frag_ctx.screen_pos = make_float2((float)x, (float)y);

          // UV coordinates (interpolated if available) - reuse already fetched UVs
          if (ctx->model->use_textures && ctx->model->vertex_data != NULL) {
            ctx->frag_ctx.uv = make_float2(
              weights.x * uv_a.x + weights.y * uv_b.x + weights.z * uv_c.x,
              weights.x * uv_a.y + weights.y * uv_b.y + weights.z * uv_c.y
            );
          } else {
            ctx->frag_ctx.uv = make_float2(0.0f, 0.0f);
          }

          ctx->frag_ctx.depth = new_depth;
          ctx->frag_ctx.view_dir = float3_normalize(float3_sub(ctx->cam->position, ctx->frag_ctx.world_pos));

          if((output_color = ctx->frag_shader->func(output_color, &ctx->frag_ctx, ctx->frag_shader->argv, ctx->frag_shader->argc))
                                          == rgb_to_u32(255, 0, 255)) {
            continue; // Discard pixel if shader returns transparent color (don't update depth)
          }

          ctx->state->framebuffer[pixel_idx] = output_color; // Draw the pixel
          ctx->state->depthbuffer[pixel_idx] = new_depth; // Update depth buffer
        }

      }
    }
  }
}

/**
* Renders a 3D model onto a 2D buffer using a basic rasterization pipeline.
* Applies transformations, projects vertices to screen space, performs simple frustum culling,
* back-face culling, and rasterizes triangles with depth testing.
*/
usize render_model(renderer_t *state, transform_t *cam, model_t *model, light_t *lights, usize light_count) {
  assert(cam != NULL);
  assert(model != NULL);
  assert(model->vertex_data != NULL);
  assert(model->num_vertices % 3 == 0); // Ensure we have complete triangles
  assert(model->frag_shader != NULL);

  fragment_shader_t *frag_shader = model->frag_shader && model->frag_shader->valid ? model->frag_shader : &default_frag_shader;
  assert(frag_shader->func != NULL);

  vertex_shader_t *vertex_shader = model->vertex_shader && model->vertex_shader->valid ? model->vertex_shader : &default_vertex_shader;
  assert(vertex_shader->func != NULL);

  if (model->use_textures) {
    assert(state->texture_atlas != NULL);
    assert(model->vertex_data != NULL); // Ensure we have vertex data with UVs
  }

  usize tris_rendered = 0;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  u64 current_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
  state->time = (current_time - state->start_time) / 1000.0f;

  // Apply vertex shader to each vertex in model space
  vertex_context_t vertex_ctx = {
    .cam_position = cam->position,
    .cam_forward = state->cam_forward,
    .cam_right = state->cam_right,
    .cam_up = state->cam_up,
    .projection_scale = state->projection_scale,
    .frustum_bound = state->frustum_bound,
    .screen_dim = state->screen_dim,
    .time = state->time,
  };

  fragment_context_t frag_ctx = {0};
  frag_ctx.time = state->time;
  frag_ctx.light = lights;
  frag_ctx.light_count = light_count;

  int total_triangles = model->num_vertices / 3;

#ifdef SHADER_WORKS_USE_PTHREADS
  // Get number of CPU cores (cross-platform)
  int num_threads = 4; // Default fallback

#ifdef _WIN32
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  num_threads = (int)sysinfo.dwNumberOfProcessors;
#elif defined(_SC_NPROCESSORS_ONLN)
  // POSIX systems (Linux, macOS)
  num_threads = (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif

  if (num_threads <= 0) num_threads = 4; // Safety fallback

  pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
  worker_thread_data_t* thread_data = malloc(num_threads * sizeof(worker_thread_data_t));
  usize* thread_counts = malloc(num_threads * sizeof(usize));

  // Shared atomic counter for work distribution
  int next_triangle = 0;

  // Create base thread context
  triangle_context_t base_ctx = {
    .state = state,
    .model = model,
    .cam = cam,
    .lights = lights,
    .light_count = light_count,
    .vertex_shader = vertex_shader,
    .frag_shader = frag_shader,
    .vertex_ctx = vertex_ctx,
    .frag_ctx = frag_ctx,
    .frustum_bound = state->frustum_bound,
    .max_depth = state->max_depth
  };

  // Launch worker threads
  for (int t = 0; t < num_threads; t++) {
    thread_data[t].base_ctx = base_ctx;
    thread_data[t].total_triangles = total_triangles;
    thread_data[t].next_triangle = &next_triangle;
    thread_data[t].triangles_rendered = &thread_counts[t];
    thread_counts[t] = 0;

    pthread_create(&threads[t], NULL, worker_thread, &thread_data[t]);
  }

  // Wait for all threads to complete
  for (int t = 0; t < num_threads; t++) {
    pthread_join(threads[t], NULL);
    tris_rendered += thread_counts[t];
  }

  free(threads);
  free(thread_data);
  free(thread_counts);

#else
  // Single-threaded fallback
  for (int tri = 0; tri < total_triangles; ++tri) {
    if (render_triangle(&(triangle_context_t){
      .state = state,
      .model = model,
      .cam = cam,
      .lights = lights,
      .light_count = light_count,
      .tri = tri,
      .vertex_shader = vertex_shader,
      .frag_shader = frag_shader,
      .vertex_ctx = vertex_ctx,
      .frag_ctx = frag_ctx,
      .frustum_bound = state->frustum_bound,
      .max_depth = state->max_depth
    })) {
      tris_rendered++;
    }
  }
#endif

  return tris_rendered;
}
