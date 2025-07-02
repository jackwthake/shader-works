#include "renderer.h"

#include <cmath>

#include "util/maths.h"
#include "device.h"
#include "display.h"
#include "scene.h"

#include "resources.inl" // Automatically generated resource definitions

using namespace std;

#define EPSILON 0.0001f
#define CUBE_VERTEX_COUNT 36


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
 * Converts world coordinates of a vertex to screen coordinates.
 * @param vertex The vertex in world coordinates.
 * @param transform The transformation to apply to the vertex.
 * @param dim The dimensions of the screen (width, height).
 * @return The screen coordinates of the vertex as a float2.
 */
static float3 vertex_to_screen(const float3 &vertex, Transform &transform, Transform &cam, float2 dim) {
  float3 vertex_world = transform.to_world_point(vertex);
  float3 vertex_view = cam.to_local_point(vertex_world);
  constexpr float fov = 1.0472; // 60 degrees in radians

  if (vertex_view.z < EPSILON) {
      return float3(0, 0, -1.0f); // Return -1.0f or similar to indicate "invalid" for the Z check
  }

  float screen_height_world = tan(fov / 2) * 2;
  float pixels_per_world_unit = dim.y / screen_height_world / vertex_view.z;

  float2 pixel_offset = float2(vertex_view.x, vertex_view.y) * pixels_per_world_unit;
  float2 vertex_screen = (dim / 2) + pixel_offset;

  return float3(vertex_screen.x, vertex_screen.y, vertex_view.z);
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


  // Loop through each triangle in the model
  for (int tri = 0; tri < CUBE_VERTEX_COUNT / 3; ++tri) {
    float3 scaled_vertex_a = model.vertices[tri * 3 + 0] * model.scale;
    float3 scaled_vertex_b = model.vertices[tri * 3 + 1] * model.scale;
    float3 scaled_vertex_c = model.vertices[tri * 3 + 2] * model.scale;

    // Get the three vertices of the current triangle
    // Transform vertices from model space to world space, then to view space, then to screen space
    float3 a = vertex_to_screen(scaled_vertex_a, model.transform, cam, screen_dim);
    float3 b = vertex_to_screen(scaled_vertex_b, model.transform, cam, screen_dim);
    float3 c = vertex_to_screen(scaled_vertex_c, model.transform, cam, screen_dim);

    // Get the UV coordinates for the current triangle's vertices
    float2 uv_a = model.uvs[tri * 3 + 0];
    float2 uv_b = model.uvs[tri * 3 + 1];
    float2 uv_c = model.uvs[tri * 3 + 2];

    // Basic frustum culling: skip triangle if any vertex is behind the camera (z <= 0)
    if (a.z <= 0 || b.z <= 0 || c.z <= 0) continue;
 
    // Perspective-correct UVs: Pre-divide UVs by their respective 1/w (a.z, b.z, c.z)
    // This gives us u/w and v/w, which we can interpolate linearly.
    // At the pixel, we'll divide by the interpolated 1/w to get the correct u and v.
    float2 uv_a_prime = uv_a / a.z; // u_a * (1/w_a), v_a * (1/w_a)
    float2 uv_b_prime = uv_b / b.z; // u_b * (1/w_b), v_b * (1/w_b)
    float2 uv_c_prime = uv_c / c.z; // u_c * (1/w_c), v_c * (1/w_c)


    // Compute triangle bounding box (clamped to screen boundaries)
    float min_x = fmaxf(0.0f, floorf(fminf(a.x, fminf(b.x, c.x))));
    float max_x = fminf(Display::width - 1, ceilf(fmaxf(a.x, fmaxf(b.x, c.x))));
    float min_y = fmaxf(0.0f, floorf(fminf(a.y, fminf(b.y, c.y))));
    float max_y = fminf(Display::height - 1, ceilf(fmaxf(a.y, fmaxf(b.y, c.y))));

    // Precompute color for this triangle (fallback if no texture)
    uint16_t flat_color = rgb_to_565(255, 0, 255);

    // Rasterize only within the computed bounding box
    for (int y = (int)min_y; y <= (int)max_y; ++y) {
      for (int x = (int)min_x; x <= (int)max_x; ++x) {
        float2 p = { (float)x, (float)y }; // Current pixel coordinate

        float3 weights; // Barycentric coordinates
        // Check if the current pixel is inside the triangle
        if (point_in_triangle({a.x, a.y}, {b.x, b.y}, {c.x, c.y}, p, weights)) {
          // Interpolate depth using barycentric coordinates (this is 1/w_interpolated)
          float new_depth = 1.0f / (weights.x / a.z + weights.y / b.z + weights.z / c.z); // new_depth is actual interpolated Z

          // Z-buffering: check if this pixel is closer than what's already drawn at this position
          int pixel_idx = y * Display::width + x;
          if (new_depth < depth_buf[pixel_idx]) {
            uint16_t final_pixel_color;

            if (use_texture && texture_data) {
              // Interpolate perspective-corrected UVs
              float interpolated_u_prime = weights.x * uv_a_prime.x + weights.y * uv_b_prime.x + weights.z * uv_c_prime.x;
              float interpolated_v_prime = weights.x * uv_a_prime.y + weights.y * uv_b_prime.y + weights.z * uv_c_prime.y;

              // Divide by interpolated 1/w (new_depth is already 1/w_interpolated)
              float final_u = interpolated_u_prime * new_depth;
              float final_v = interpolated_v_prime * new_depth;

              // Map normalized UVs [0.0, 1.0] to texture pixel coordinates [0, tex_width-1], [0, tex_height-1]
              int tex_x = static_cast<int>(final_u * tex_width);
              int tex_y = static_cast<int>(final_v * tex_height);

              // Clamp texture coordinates to avoid out-of-bounds access (optional, but good practice)
              tex_x = constrain(tex_x, 0, tex_width - 1);
              tex_y = constrain(tex_y, 0, tex_height - 1);

              final_pixel_color = texture_data[tex_y * tex_width + tex_x];
            } else {
              final_pixel_color = flat_color; // Use flat color as fallback
            }
            
            // Apply distance-based fog
            float fog_factor = (new_depth - FOG_START) / (FOG_END - FOG_START);
            fog_factor = constrain(fog_factor, 0.0f, 1.0f);

            // Extract RGB components from BGR565 format correctly
            uint8_t b = (final_pixel_color & 0x1F) << 3;        // Blue: bits 0-4, expand to 8-bit
            uint8_t g = ((final_pixel_color >> 5) & 0x3F) << 2; // Green: bits 5-10, expand to 8-bit  
            uint8_t r = ((final_pixel_color >> 11) & 0x1F) << 3; // Red: bits 11-15, expand to 8-bit

            // Fog color (light gray) - 8-bit values
            constexpr uint8_t fog_r = 175;
            constexpr uint8_t fog_g = 175;
            constexpr uint8_t fog_b = 255;

            // Interpolate between original color and fog color
            r = (uint8_t)(r * (1.0f - fog_factor) + fog_r * fog_factor);
            g = (uint8_t)(g * (1.0f - fog_factor) + fog_g * fog_factor);
            b = (uint8_t)(b * (1.0f - fog_factor) + fog_b * fog_factor);

            final_pixel_color = rgb_to_565(r, g, b);
            buff[pixel_idx] = final_pixel_color; // Draw the pixel
            depth_buf[pixel_idx] = new_depth; // Update depth buffer
          }
        }
      }
    }
  }
}