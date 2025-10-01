#include "renderer.h"

#include <cmath>

#include "util/maths.h"
#include "device.h"
#include "display.h"
#include "scene.h"

#include "resources.inl" // Automatically generated resource definitions

using namespace std;

constexpr float EPSILON = 0.0001f;
constexpr int CUBE_VERTEX_COUNT = 36;


// --- Constants for the Atlas ---
constexpr int ATLAS_WIDTH_PX = 80;
constexpr int ATLAS_HEIGHT_PX = 24;
constexpr int TILE_WIDTH_PX = 8;
constexpr int TILE_HEIGHT_PX = 8;

// Derived constants
constexpr int TILES_PER_ROW = ATLAS_WIDTH_PX / TILE_WIDTH_PX;
constexpr int TILES_PER_COL = ATLAS_HEIGHT_PX / TILE_HEIGHT_PX;
constexpr int TOTAL_TILES = TILES_PER_ROW * TILES_PER_COL;
constexpr int TILE_PIXEL_COUNT = TILE_WIDTH_PX * TILE_HEIGHT_PX;


 /**
 * packs 3 8 bit RGB values into a 16 bit BGR565 value.
 * The 16 bit value is packed as follows:
 *  - 5 bits for blue (B)
 *  - 6 bits for green (G)
 *  - 5 bits for red (R) 
 */
static uint16_t rgb_to_565(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t BGRColor = b >> 3;
    BGRColor         |= (g & 0xFC) << 3;
    BGRColor         |= (r & 0xF8) << 8;

    return BGRColor;
}


/**
 * Calculates the signed area of a triangle defined by three points.
 * The sign indicates the orientation of the triangle (positive for counter-clockwise, negative for clockwise).
 * @param a The first vertex of the triangle.
 * @param b The second vertex of the triangle.
 * @param c The third vertex of the triangle.
 * @return The signed area of the triangle.
 */
static float signed_triangle_area(const float2& a, const float2& b, const float2& c) {
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
static bool point_in_triangle(const float2& a, const float2& b, const float2& c, const float2& p, float3 &weights) {
  float area_abp = signed_triangle_area(a, b, p);
  float area_bcp = signed_triangle_area(b, c, p);
  float area_cap = signed_triangle_area(c, a, p);
  bool in_triangle = (area_abp >= 0 && area_bcp >= 0 && area_cap >= 0) ||
                 (area_abp <= 0 && area_bcp <= 0 && area_cap <= 0);

  float inv_area_sum = 1.0f / (area_abp + area_bcp + area_cap);
  float weight_a = area_bcp * inv_area_sum;
  float weight_b = area_cap * inv_area_sum;
  float weight_c = area_abp * inv_area_sum;
  weights = float3(weight_a, weight_b, weight_c);

  return in_triangle;
}


/**
 * Computes the UV coordinates for a tile in the texture atlas.
 * @param uvs The vector to store the computed UV coordinates.
 * @param tile_id The ID of the tile to compute UVs for.
 */
void compute_uv_coords(std::vector<float2>& uvs, int tile_id) {
  if (tile_id < 0 || tile_id >= TOTAL_TILES) {
    Serial.printf("Error: Tile ID %d is out of bounds. Must be between 0 and %d\n", tile_id, (TOTAL_TILES - 1));
  }

  // Calculate the tile's top-left pixel corner in the atlas.
  float tile_x_start = static_cast<float>((tile_id % TILES_PER_ROW) * TILE_WIDTH_PX);
  float tile_y_start = static_cast<float>((tile_id / TILES_PER_ROW) * TILE_HEIGHT_PX);

  // Convert pixel coordinates to normalized UV coordinates [0.0, 1.0] by casting constants to float.
  float u_start = tile_x_start / static_cast<float>(ATLAS_WIDTH_PX);
  float v_start = tile_y_start / static_cast<float>(ATLAS_HEIGHT_PX);
  float u_end = (tile_x_start + static_cast<float>(TILE_WIDTH_PX)) / static_cast<float>(ATLAS_WIDTH_PX);
  float v_end = (tile_y_start + static_cast<float>(TILE_HEIGHT_PX)) / static_cast<float>(ATLAS_HEIGHT_PX);

  // Define the 4 UV corners for the tile.
  float2 uv_top_left     = {u_start, v_start};
  float2 uv_top_right    = {u_end,   v_start};
  float2 uv_bottom_left  = {u_start, v_end};
  float2 uv_bottom_right = {u_end,   v_end};

  // Add UVs for the first triangle (v0, v1, v2)
  uvs.push_back(uv_top_left);     // Corresponds to vertex v0
  uvs.push_back(uv_bottom_left);  // Corresponds to vertex v1
  uvs.push_back(uv_bottom_right); // Corresponds to vertex v2

  // Add UVs for the second triangle (v0, v2, v3)
  uvs.push_back(uv_top_left);     // Corresponds to vertex v0
  uvs.push_back(uv_bottom_right); // Corresponds to vertex v2
  uvs.push_back(uv_top_right);    // Corresponds to vertex v3
}


/**
 * Renders a 3D model onto a 2D buffer using a basic rasterization pipeline.
 * Applies transformations, projects vertices to screen space, performs simple frustum culling,
 * back-face culling, and rasterizes triangles with depth testing.
 *
 * @param buff A pointer to the 16-bit color buffer (e.g., Device::back_buffer).
 * @param camera The camera's transform (position and orientation).
 * @param model The Model to render, containing vertices, colors, and its own transform.
 */
void render_model(uint16_t *buff, float *depth_buf, Transform &cam, Model &model) {
  float2 screen_dim = { (float)Display::width, (float)Display::height };

  const uint16_t* texture_data = files[0].data; // Assuming the texture atlas is the first resource
  int tex_width = ATLAS_WIDTH_PX;
  int tex_height = ATLAS_HEIGHT_PX;
  bool use_texture = true;

  // Pre-compute projection constants
  constexpr float fov = 1.0472f; // 60 degrees in radians
  const float screen_height_world = tan(fov / 2) * 2;
  const float projection_scale = screen_dim.y / screen_height_world;


  // Loop through each triangle in the model
  for (int tri = 0; tri < CUBE_VERTEX_COUNT / 3; ++tri) {
    float3 scaled_vertex_a = model.vertices[tri * 3 + 0] * model.scale;
    float3 scaled_vertex_b = model.vertices[tri * 3 + 1] * model.scale;
    float3 scaled_vertex_c = model.vertices[tri * 3 + 2] * model.scale;

    // Transform vertices from model space to world space, then to view space
    float3 world_a = model.transform.to_world_point(scaled_vertex_a);
    float3 world_b = model.transform.to_world_point(scaled_vertex_b);
    float3 world_c = model.transform.to_world_point(scaled_vertex_c);
    
    float3 view_a = cam.to_local_point(world_a);
    float3 view_b = cam.to_local_point(world_b);
    float3 view_c = cam.to_local_point(world_c);

    // Basic frustum culling: skip triangle if all vertices are behind camera OR outside frustum
    if (view_a.z <= 0 && view_b.z <= 0 && view_c.z <= 0) continue;
    
    // Additional frustum culling - check if triangle is completely outside view frustum
    float frustum_bound = tan(fov / 2) * 1.4f; // Add small margin
    bool outside_left   = (view_a.x < -view_a.z * frustum_bound && view_b.x < -view_b.z * frustum_bound && view_c.x < -view_c.z * frustum_bound);
    bool outside_right  = (view_a.x >  view_a.z * frustum_bound && view_b.x >  view_b.z * frustum_bound && view_c.x >  view_c.z * frustum_bound);
    bool outside_top    = (view_a.y >  view_a.z * frustum_bound && view_b.y >  view_b.z * frustum_bound && view_c.y >  view_c.z * frustum_bound);
    bool outside_bottom = (view_a.y < -view_a.z * frustum_bound && view_b.y < -view_b.z * frustum_bound && view_c.y < -view_c.z * frustum_bound);
    
    if (outside_left || outside_right || outside_top || outside_bottom) continue;

    // Use pre-computed projection constants
    float pixels_per_world_unit_a = projection_scale / view_a.z;
    float pixels_per_world_unit_b = projection_scale / view_b.z;
    float pixels_per_world_unit_c = projection_scale / view_c.z;
    
    float2 pixel_offset_a = float2(view_a.x, view_a.y) * pixels_per_world_unit_a;
    float2 pixel_offset_b = float2(view_b.x, view_b.y) * pixels_per_world_unit_b;
    float2 pixel_offset_c = float2(view_c.x, view_c.y) * pixels_per_world_unit_c;
    
    float2 screen_a = (screen_dim / 2) + pixel_offset_a;
    float2 screen_b = (screen_dim / 2) + pixel_offset_b;
    float2 screen_c = (screen_dim / 2) + pixel_offset_c;
    
    float3 a = float3(screen_a.x, screen_a.y, view_a.z);
    float3 b = float3(screen_b.x, screen_b.y, view_b.z);
    float3 c = float3(screen_c.x, screen_c.y, view_c.z);

    // Proper back-face culling using world-space triangle normal and camera forward vector
    float3 world_edge1 = world_b - world_a;
    float3 world_edge2 = world_c - world_a;
    
    // Compute triangle normal (cross product)
    float3 triangle_normal;
    triangle_normal.x = world_edge1.y * world_edge2.z - world_edge1.z * world_edge2.y;
    triangle_normal.y = world_edge1.z * world_edge2.x - world_edge1.x * world_edge2.z;
    triangle_normal.z = world_edge1.x * world_edge2.y - world_edge1.y * world_edge2.x;
    
    // Get camera forward vector (in world space)
    float3 cam_right, cam_up, cam_forward;
    cam.get_basis_vectors(cam_right, cam_up, cam_forward);
    
    // Check if triangle is facing away from camera using dot product
    float dot_product = triangle_normal.x * cam_forward.x + triangle_normal.y * cam_forward.y + triangle_normal.z * cam_forward.z;
    if (dot_product < 0) continue; // Triangle is facing away from camera

    // Get the UV coordinates for the current triangle's vertices
    float2 uv_a = model.uvs[tri * 3 + 0];
    float2 uv_b = model.uvs[tri * 3 + 1];
    float2 uv_c = model.uvs[tri * 3 + 2];
 
    // Perspective-correct UVs: Pre-divide UVs by their respective 1/w
    float2 uv_a_prime = uv_a / a.z;
    float2 uv_b_prime = uv_b / b.z;
    float2 uv_c_prime = uv_c / c.z;

    // Compute triangle bounding box (clamped to screen boundaries)
    float min_x = fmaxf(0.0f, floorf(fminf(a.x, fminf(b.x, c.x))));
    float max_x = fminf(Display::width - 1, ceilf(fmaxf(a.x, fmaxf(b.x, c.x))));
    float min_y = fmaxf(0.0f, floorf(fminf(a.y, fminf(b.y, c.y))));
    float max_y = fminf(Display::height - 1, ceilf(fmaxf(a.y, fmaxf(b.y, c.y))));

    // Early exit if triangle is completely outside screen bounds
    if (min_x > max_x || min_y > max_y) continue;

    // Precompute color for this triangle (fallback if no texture)
    uint16_t flat_color = rgb_to_565(255, 0, 255);

    // Precompute values outside the pixel loop
    const float inv_tex_width = 1.0f / tex_width;
    const float inv_tex_height = 1.0f / tex_height;
    const int tex_width_minus_1 = tex_width - 1;
    const int tex_height_minus_1 = tex_height - 1;

    // Rasterize only within the computed bounding box
    for (int y = (int)min_y; y <= (int)max_y; ++y) {
      int pixel_base = y * Display::width; // Precompute row offset

      for (int x = (int)min_x; x <= (int)max_x; ++x) {
        float2 p = { (float)x, (float)y }; // Current pixel coordinate

        float3 weights; // Barycentric coordinates
        // Check if the current pixel is inside the triangle
        if (point_in_triangle({a.x, a.y}, {b.x, b.y}, {c.x, c.y}, p, weights)) {
          // Interpolate depth using barycentric coordinates
          float new_depth = 1.0f / (weights.x / a.z + weights.y / b.z + weights.z / c.z);

          // Z-buffering: check if this pixel is closer than what's already drawn at this position
          int pixel_idx = pixel_base + x; // Use precomputed row offset
          if (new_depth < depth_buf[pixel_idx]) {
            uint16_t final_pixel_color;

            if (use_texture && texture_data) {
              // Interpolate perspective-corrected UVs
              float interpolated_u_prime = weights.x * uv_a_prime.x + weights.y * uv_b_prime.x + weights.z * uv_c_prime.x;
              float interpolated_v_prime = weights.x * uv_a_prime.y + weights.y * uv_b_prime.y + weights.z * uv_c_prime.y;

              // Divide by interpolated 1/w to get correct perspective UVs
              float final_u = interpolated_u_prime * new_depth;
              float final_v = interpolated_v_prime * new_depth;

              // Map normalized UVs [0.0, 1.0] to texture pixel coordinates (optimized)
              int tex_x = (int)(final_u * tex_width);
              int tex_y = (int)(final_v * tex_height);

              // Fast clamp using bit operations and conditionals
              tex_x = (tex_x < 0) ? 0 : ((tex_x > tex_width_minus_1) ? tex_width_minus_1 : tex_x);
              tex_y = (tex_y < 0) ? 0 : ((tex_y > tex_height_minus_1) ? tex_height_minus_1 : tex_y);

              final_pixel_color = texture_data[tex_y * tex_width + tex_x];
              
              // Apply basic shading to north, northwest, and bottom faces
              // Face order: bottom(0-1), top(2-3), front(4-5), back(6-7), right(8-9), left(10-11)
              // Bottom face = triangles 0-1, North face = back face (triangles 6-7), Northwest face = left face (triangles 10-11)
              if (tri == 0 || tri == 1 || tri == 6 || tri == 7 || tri == 10 || tri == 11) {
                // Extract RGB components and darken by ~25%
                uint8_t shade_r = ((final_pixel_color >> 11) & 0x1F);
                uint8_t shade_g = ((final_pixel_color >> 5) & 0x3F);
                uint8_t shade_b = (final_pixel_color & 0x1F);
                
                // Apply 0.75 multiplier for shading (25% darker)
                shade_r = (shade_r * 3) >> 2;  // * 0.75
                shade_g = (shade_g * 3) >> 2;  // * 0.75  
                shade_b = (shade_b * 3) >> 2;  // * 0.75
                
                // Repack into RGB565
                final_pixel_color = (shade_r << 11) | (shade_g << 5) | shade_b;
              }
            } else {
              final_pixel_color = flat_color; // Use flat color as fallback
            }
            
            // Apply distance-based fog
            float fog_factor = (new_depth - FOG_START) / (FOG_END - FOG_START);
            fog_factor = constrain(fog_factor, 0.0f, 1.0f);

            // For full fog (fog_factor >= 0.99)
            if (fog_factor >= 0.99f) {
              final_pixel_color = 0x8E1C;
            } else {
              // Extract RGB components from BGR565 format
              uint8_t b = (final_pixel_color & 0x1F) << 3;        // Blue: bits 0-4, expand to 8-bit
              uint8_t g = ((final_pixel_color >> 5) & 0x3F) << 2; // Green: bits 5-10, expand to 8-bit  
              uint8_t r = ((final_pixel_color >> 11) & 0x1F) << 3; // Red: bits 11-15, expand to 8-bit

              // Fog color
              constexpr uint8_t fog_r = 142;
              constexpr uint8_t fog_g = 193;
              constexpr uint8_t fog_b = 230;

              // Interpolate between original color and fog color
              r = (uint8_t)(r * (1.0f - fog_factor) + fog_r * fog_factor);
              g = (uint8_t)(g * (1.0f - fog_factor) + fog_g * fog_factor);
              b = (uint8_t)(b * (1.0f - fog_factor) + fog_b * fog_factor);
              final_pixel_color = rgb_to_565(r, g, b);
            }

            buff[pixel_idx] = final_pixel_color; // Draw the pixel
            depth_buf[pixel_idx] = new_depth; // Update depth buffer
          }
        }
      }
    }
  }
}