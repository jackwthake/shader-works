#include "renderer.h"

#include <cmath>

#include "device.h"
#include "util/helpers.h"
#include "util/maths.h"

using namespace std;


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
static float3 vertex_to_screen(const float3 &vertex, Transform &transform, float2 dim) {
  float3 vertex_world = transform.to_world_point(vertex);
  float fov = 1.0472; // 60 degrees in radians

  float screen_height_world = tan(fov / 2) * 2;
  float pixels_per_world_unit = dim.y / screen_height_world / vertex_world.z;

  float2 screen_pos;
  screen_pos.x = vertex_world.x * pixels_per_world_unit;
  screen_pos.y = vertex_world.y * pixels_per_world_unit;

  float2 vertex_screen = (dim / 2) + screen_pos;

  return float3(vertex_screen.x, vertex_screen.y, vertex_world.z);
}


void render_model(uint16_t *buff, Model &model) {
  // For each triangle in the model
  size_t triangle_count = model.vertices.size() / 3;
  float2 screen_dim(Device::width, Device::height);
  for (size_t tri = 0; tri < triangle_count; ++tri) {
    float3 a = vertex_to_screen(model.vertices[tri * 3 + 0], model.transform, screen_dim);
    float3 b = vertex_to_screen(model.vertices[tri * 3 + 1], model.transform, screen_dim);
    float3 c = vertex_to_screen(model.vertices[tri * 3 + 2], model.transform, screen_dim);

    // Compute triangle bounding box (clamped to screen)
    float min_x = fmaxf(0.0f, floorf(fminf(a.x, fminf(b.x, c.x))));
    float max_x = fminf(Device::width - 1, ceilf(fmaxf(a.x, fmaxf(b.x, c.x))));
    float min_y = fmaxf(0.0f, floorf(fminf(a.y, fminf(b.y, c.y))));
    float max_y = fminf(Device::height - 1, ceilf(fmaxf(a.y, fmaxf(b.y, c.y))));

    // Precompute color for this triangle
    int color_idx = tri < model.cols.size() ? tri : 0;
    uint16_t color = rgb_to_565(model.cols[color_idx].x, model.cols[color_idx].y, model.cols[color_idx].z);

    // Rasterize only within bounding box
    for (int y = (int)min_y; y <= (int)max_y; ++y) {
      for (int x = (int)min_x; x <= (int)max_x; ++x) {
        float3 weights;
        if (point_in_triangle(float2(a.x, a.y), float2(b.x, b.y), float2(c.x, c.y), float2(x, y), weights)) {
          float3 depths(a.z, b.z, c.z);
          float depth = float3::dot(depths, weights);
          if (depth < 0 || depth > Device::max_depth) continue;
          int idx = x + Device::width * y;
          if (depth < Device::depth_buffer[idx]) {
            buff[idx] = color;
            Device::depth_buffer[idx] = depth;
          }
        }
      }
    }
  }
}