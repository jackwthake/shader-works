#include "renderer.h"

#include <cmath>

#include "device.h"
#include "util/helpers.h"
#include "util/maths.h"

using namespace std;

#define EPSILON 0.0001f


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
 * Renders a 3D model onto a 2D buffer using a basic rasterization pipeline.
 * Applies transformations, projects vertices to screen space, performs simple frustum culling,
 * back-face culling, and rasterizes triangles with depth testing.
 *
 * @param buff A pointer to the 16-bit color buffer (e.g., Device::back_buffer).
 * @param camera The camera's transform (position and orientation).
 * @param model The Model to render, containing vertices, colors, and its own transform.
 */
void render_model(uint16_t *buff, Transform &cam, Model &model) {
  float2 screen_dim = { (float)Device::width, (float)Device::height };

  // Get texture atlas data if a valid ID is provided
  const Resource_entry_t* texture_res = nullptr;
  const uint16_t* texture_data = nullptr;
  int tex_width = 0;
  int tex_height = 0;
  bool use_texture = false;

  if (model.texture_atlas_id != INVALID_RESOURCE) {
    texture_res = Device::manager.get_resource(model.texture_atlas_id);
    if (texture_res && texture_res->type == BITMAP_FILE) {
      texture_data = reinterpret_cast<const uint16_t*>(texture_res->data);
      tex_width = texture_res->width;
      tex_height = texture_res->height;
      use_texture = true;
    } else {
        Serial.printf("Warning: Model has invalid texture_atlas_id %d or resource is not a BITMAP_FILE.\n", model.texture_atlas_id);
    }
  }


  // Loop through each triangle in the model
  for (int tri = 0; tri < model.vertices.size() / 3; ++tri) {
    // Get the three vertices of the current triangle
    // Transform vertices from model space to world space, then to view space, then to screen space
    float3 a = vertex_to_screen(model.vertices[tri * 3 + 0], model.transform, cam, screen_dim);
    float3 b = vertex_to_screen(model.vertices[tri * 3 + 1], model.transform, cam, screen_dim);
    float3 c = vertex_to_screen(model.vertices[tri * 3 + 2], model.transform, cam, screen_dim);

    // Get the UV coordinates for the current triangle's vertices
    float2 uv_a = model.uvs[tri * 3 + 0];
    float2 uv_b = model.uvs[tri * 3 + 1];
    float2 uv_c = model.uvs[tri * 3 + 2];

    // Basic frustum culling: skip triangle if any vertex is behind the camera (z <= 0)
    // Note: a.z, b.z, c.z here are 1/w values from vertex_to_screen
    if (a.z <= 0 || b.z <= 0 || c.z <= 0) continue; 

    Serial.printf("Processing Triangle %d (Orig Vtx Indices: %d, %d, %d)\n", tri, tri * 3, tri * 3 + 1, tri * 3 + 2);
    Serial.printf("  Screen coords: A(%d,%d,%d), B(%d,%d,%d), C(%d,%d,%d)\n", (int)(a.x * 100), (int)(a.y * 100), (int)(a.z * 100), (int)(b.x * 100), (int)(b.y * 100), (int)(b.z * 100), (int)(c.x * 100), (int)(c.y * 100), (int)(c.z * 100));

    // Back-face culling implementation: skip back-facing or degenerate triangles
    float projected_area = signed_triangle_area({a.x, a.y}, {b.x, b.y}, {c.x, c.y});
    Serial.printf("  Signed Triangle Area: %d\n", (int)(projected_area * 100));
    // if (projected_area >= 0) { // <-- problematic potentially
    //     Serial.printf("  Triangle %d CULLED (Area <= 0)\n", tri);
    //     continue; 
    // }

    // Perspective-correct UVs: Pre-divide UVs by their respective 1/w (a.z, b.z, c.z)
    // This gives us u/w and v/w, which we can interpolate linearly.
    // At the pixel, we'll divide by the interpolated 1/w to get the correct u and v.
    float2 uv_a_prime = uv_a / a.z; // u_a * (1/w_a), v_a * (1/w_a)
    float2 uv_b_prime = uv_b / b.z; // u_b * (1/w_b), v_b * (1/w_b)
    float2 uv_c_prime = uv_c / c.z; // u_c * (1/w_c), v_c * (1/w_c)


    // Compute triangle bounding box (clamped to screen boundaries)
    float min_x = fmaxf(0.0f, floorf(fminf(a.x, fminf(b.x, c.x))));
    float max_x = fminf(Device::width - 1, ceilf(fmaxf(a.x, fmaxf(b.x, c.x))));
    float min_y = fmaxf(0.0f, floorf(fminf(a.y, fminf(b.y, c.y))));
    float max_y = fminf(Device::height - 1, ceilf(fmaxf(a.y, fmaxf(b.y, c.y))));

    // Precompute color for this triangle (fallback if no texture)
    int color_idx = tri < model.cols.size() ? tri : 0;
    uint16_t flat_color = rgb_to_565(model.cols[color_idx].x, model.cols[color_idx].y, model.cols[color_idx].z);

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
          int pixel_idx = y * Device::width + x;
          if (new_depth < Device::depth_buffer[pixel_idx]) {
            uint16_t final_pixel_color;

            if (use_texture && texture_data) {
              // Interpolate perspective-corrected UVs
              float interpolated_u_prime = weights.x * uv_a_prime.x + weights.y * uv_b_prime.x + weights.z * uv_c_prime.x;
              float interpolated_v_prime = weights.x * uv_a_prime.y + weights.y * uv_b_prime.y + weights.z * uv_c_prime.y;

              // Divide by interpolated 1/w (new_depth is already 1/w_interpolated)
              // NOTE: new_depth is the actual interpolated Z-value here, not 1/w.
              // We need the interpolated 1/w which is 1.0f / new_depth.
              // So, u = (u/w)_interp / (1/w)_interp = interpolated_u_prime / (1.0f / new_depth)
              // This simplifies to u = interpolated_u_prime * new_depth;
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
            
            buff[pixel_idx] = final_pixel_color; // Draw the pixel
            Device::depth_buffer[pixel_idx] = new_depth; // Update depth buffer
          }
        }
      }
    }
  }
}