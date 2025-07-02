#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <stdint.h>

#include "display.h"
#include "util/maths.h"

constexpr float MAX_DEPTH = 25.0f; // Maximum depth value for the depth buffer
constexpr float FOG_START = 5.0f;   // Start of fog effect
constexpr float FOG_END = 20.0f;    // End of fog effect

void compute_uv_coords(std::vector<float2>& uvs, int tile_id);
void render_model(uint16_t *buff, float *depth_buf, Transform &camera, struct Model &model);

#endif // RENDERER_H