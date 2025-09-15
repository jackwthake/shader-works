#include "renderer.h"

#include <math.h>
#include <assert.h>

#include "maths.h"

float3 cube_vertices[CUBE_VERTEX_COUNT] = {
  {-0.500000, 0.500000, -0.500000}, {-0.500000, 0.500000, 0.500000}, {0.500000, 0.500000, 0.500000}, 
  {-0.500000, 0.500000, -0.500000}, {0.500000, 0.500000, 0.500000}, {0.500000, 0.500000, -0.500000}, 
  {0.500000, -0.500000, -0.500000}, {0.500000, -0.500000, 0.500000}, {-0.500000, -0.500000, 0.500000}, 
  {0.500000, -0.500000, -0.500000}, {-0.500000, -0.500000, 0.500000}, {-0.500000, -0.500000, -0.500000}, 
  {0.500000, 0.500000, 0.500000}, {0.500000, -0.500000, 0.500000}, {-0.500000, -0.500000, 0.500000}, 
  {0.500000, 0.500000, 0.500000}, {-0.500000, -0.500000, 0.500000}, {-0.500000, 0.500000, 0.500000}, 
  {-0.500000, 0.500000, -0.500000}, {-0.500000, -0.500000, -0.500000}, {0.500000, -0.500000, -0.500000}, 
  {-0.500000, 0.500000, -0.500000}, {0.500000, -0.500000, -0.500000}, {0.500000, 0.500000, -0.500000}, 
  {0.500000, 0.500000, -0.500000}, {0.500000, -0.500000, -0.500000}, {0.500000, -0.500000, 0.500000}, 
  {0.500000, 0.500000, -0.500000}, {0.500000, -0.500000, 0.500000}, {0.500000, 0.500000, 0.500000}, 
  {-0.500000, 0.500000, 0.500000}, {-0.500000, -0.500000, 0.500000}, {-0.500000, -0.500000, -0.500000}, 
  {-0.500000, 0.500000, 0.500000}, {-0.500000, -0.500000, -0.500000}, {-0.500000, 0.500000, -0.500000}
};


//TODO: Verify this format, the colors don't seem right
static u32 rgb_to_888(const u8 r, const u8 g, const u8 b) {
  return (0xFF << 24) | (r << 16) | (g << 8) | (b << 0);
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
  
  float inv_area_sum = 1.0f / (area_abp + area_bcp + area_cap);
  float weight_a = area_bcp * inv_area_sum;
  float weight_b = area_cap * inv_area_sum;
  float weight_c = area_abp * inv_area_sum;
  
  assert(weights != NULL);
  *weights = (float3){ weight_a, weight_b, weight_c };
  
  return in_triangle;
}


static float3 transform_vector(float3 ihat, float3 jhat, float3 khat, float3 vec) {
  return (float3){
    vec.x * ihat.x + vec.y * jhat.x + vec.z * khat.x,
    vec.x * ihat.y + vec.y * jhat.y + vec.z * khat.y,
    vec.x * ihat.z + vec.y * jhat.z + vec.z * khat.z
  };
}

static void transform_get_basis_vectors(transform_t *t, float3 *ihat, float3 *jhat, float3 *khat) {
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

static float3 transform_to_world(transform_t *t, float3 p) {
  assert(t != NULL);

  float3 ihat, jhat, khat;
  transform_get_basis_vectors(t, &ihat, &jhat, &khat);
  
  // Transform the point and then translate by position
  float3 rotated = transform_vector(ihat, jhat, khat, p);
  return float3_add(rotated, t->position);
}

static float3 transform_to_local_point(transform_t *t, float3 p) {
  assert(t != NULL);

  float3 ihat, jhat, khat;
  transform_get_inverse_basis_vectors(t, &ihat, &jhat, &khat);
  
  // Translate first, then rotate
  float3 p_rel = float3_sub(p, t->position);
  return transform_vector(ihat, jhat, khat, p_rel);
}

// Apply basic shading to north, northwest, and bottom faces
// TODO: Only some of the faces are shaded, need to investigate why
static void apply_side_shading(u32 *color, int tri) {
  assert(color != NULL);
  // Face order: bottom(0-1), top(2-3), front(4-5), back(6-7), right(8-9), left(10-11)
  // Bottom face = triangles 0-1, North face = back face (triangles 6-7), Northwest face = left face (triangles 10-11)
  if (tri == 0 || tri == 1 || tri == 6 || tri == 7 || tri == 10 || tri == 11) {
    // Extract RGB components and darken by ~25%
    uint8_t shade_r = ((*color >> 16) & 0xFF);
    uint8_t shade_g = ((*color >> 8) & 0xFF);
    uint8_t shade_b = (*color & 0xFF);
    
    // Apply 0.75 multiplier for shading (25% darker)
    shade_r = (shade_r * 3) >> 2;  // * 0.75
    shade_g = (shade_g * 3) >> 2;  // * 0.75  
    shade_b = (shade_b * 3) >> 2;  // * 0.75
    
    // Repack into RGB888
    *color = rgb_to_888(shade_r, shade_g, shade_b);
  }
}

static void apply_fog(u32 *color, f32 depth) {
  assert(color != NULL);
  f32 fog_factor = (depth - FOG_START) / (FOG_END - FOG_START);
  
  // Clamp fog factor
  if (fog_factor < 0.0f) fog_factor = 0.0f;
  if (fog_factor > 1.0f) fog_factor = 1.0f;
  
  // For full fog (fog_factor >= 0.99)
  if (fog_factor >= 0.99f) {
    *color = FOG_COLOR;
  } else {
    uint8_t r = (*color >> 16) & 0xFF;
    uint8_t g = (*color >> 8) & 0xFF;  
    uint8_t b = (*color & 0xFF);
    
    // Fog color
    uint8_t fog_r = 52;
    uint8_t fog_g = 72;
    uint8_t fog_b = 102;
    
    // Interpolate between original color and fog color
    r = (uint8_t)(r * (1.0f - fog_factor) + fog_r * fog_factor);
    g = (uint8_t)(g * (1.0f - fog_factor) + fog_g * fog_factor);
    b = (uint8_t)(b * (1.0f - fog_factor) + fog_b * fog_factor);
    *color = rgb_to_888(r, g, b);
  }
}

u32 default_shader(u32 input_color, void *args, usize argc) {
  return input_color;
}

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

void update_camera(game_state_t *state, transform_t *cam) {
  transform_get_basis_vectors(cam, &state->cam_right, &state->cam_up, &state->cam_forward);
}

/**
* Renders a 3D model onto a 2D buffer using a basic rasterization pipeline.
* Applies transformations, projects vertices to screen space, performs simple frustum culling,
* back-face culling, and rasterizes triangles with depth testing.
*/
void render_model(game_state_t *state, transform_t *cam, model_t *model, shader_func frag_shader, void *shader_args, usize shader_argc) {
  assert(state != NULL);
  assert(cam != NULL);
  assert(model != NULL);
  assert(model->vertices != NULL);
  assert(model->num_vertices % 3 == 0); // Ensure we have complete triangles

  if (frag_shader == NULL) {
    frag_shader = default_shader;
  }

  // Loop through each triangle in the model
  for (int tri = 0; tri < model->num_vertices / 3; ++tri) {
    // Transform vertices from model space to world space, then to view space
    float3 world_a = transform_to_world(&model->transform, model->vertices[tri * 3 + 0]);
    float3 world_b = transform_to_world(&model->transform, model->vertices[tri * 3 + 1]);
    float3 world_c = transform_to_world(&model->transform, model->vertices[tri * 3 + 2]);

    float3 view_a = transform_to_local_point(cam, world_a);
    float3 view_b = transform_to_local_point(cam, world_b);
    float3 view_c = transform_to_local_point(cam, world_c);
    
    // Basic frustum culling: skip triangle if all vertices are behind camera
    if (view_a.z <= 0 && view_b.z <= 0 && view_c.z <= 0) continue;
    
    // Additional frustum culling - check if triangle is completely outside view frustum
    bool outside_left   = (view_a.x < -view_a.z * state->frustum_bound && view_b.x < -view_b.z * state->frustum_bound && view_c.x < -view_c.z * state->frustum_bound);
    bool outside_right  = (view_a.x >  view_a.z * state->frustum_bound && view_b.x >  view_b.z * state->frustum_bound && view_c.x >  view_c.z * state->frustum_bound);
    bool outside_top    = (view_a.y >  view_a.z * state->frustum_bound && view_b.y >  view_b.z * state->frustum_bound && view_c.y >  view_c.z * state->frustum_bound);
    bool outside_bottom = (view_a.y < -view_a.z * state->frustum_bound && view_b.y < -view_b.z * state->frustum_bound && view_c.y < -view_c.z * state->frustum_bound);
    
    if (outside_left || outside_right || outside_top || outside_bottom) continue;
    
    // Proper back-face culling using triangle normal and view direction
    float3 world_edge1 = float3_sub(world_b, world_a);
    float3 world_edge2 = float3_sub(world_c, world_a);
    
    // Compute triangle normal (cross product)
    float3 triangle_normal;
    triangle_normal.x = world_edge1.y * world_edge2.z - world_edge1.z * world_edge2.y;
    triangle_normal.y = world_edge1.z * world_edge2.x - world_edge1.x * world_edge2.z;
    triangle_normal.z = world_edge1.x * world_edge2.y - world_edge1.y * world_edge2.x;
    
    // Vector from triangle center to camera
    float3 triangle_center = float3_scale(float3_add(float3_add(world_a, world_b), world_c), 1.0f/3.0f);
    float3 view_direction = float3_sub(cam->position, triangle_center);
    
    // Check if triangle is facing toward camera
    // If dot product > 0, triangle normal points toward camera (front-facing)
    float dot_product = float3_dot(triangle_normal, view_direction);
    //TODO: This has been causing so many issues with faces disappearing, need to investigate further
    // if (dot_product <= 0) continue; // Triangle is facing away from camera
    
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
          float new_depth = 1.0f / (weights.x / a.z + weights.y / b.z + weights.z / c.z);
          
          // Z-buffering: check if this pixel is closer than what's already drawn at this position
          int pixel_idx = pixel_base + x; // Use precomputed row offset
          if (new_depth < state->depthbuffer[pixel_idx]) {
            uint32_t output_color;
            
            if (state->use_textures && state->texture_atlas) {
              // Texture code would go here
              output_color = flat_color;
            } else {
              output_color = flat_color; // Use flat color if no texture
            }

            output_color = frag_shader(output_color, shader_args, shader_argc);
            apply_side_shading(&output_color, tri);
            apply_fog(&output_color, new_depth);
            
            state->framebuffer[pixel_idx] = output_color; // Draw the pixel
            state->depthbuffer[pixel_idx] = new_depth; // Update depth buffer
          }
        }
      }
    }
  }
}