#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <stdint.h>

#include "util/vec.h"
#include "util/transform.h"

struct Model {
  std::vector<float3> vertices; // List of vertices
  std::vector<float3> cols; // List of vertex colors

  Transform transform;
};

void draw_fill_triangle(uint16_t* framebuffer, float3 p1, float3 p2, float3 p3, uint16_t color);
void render_model(uint16_t * &buff, Model &model);

#endif // RENDERER_H