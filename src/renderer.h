#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <stdint.h>

#include "display.h"
#include "resource_manager.h"

#include "util/maths.h"
#include "util/model.h"

constexpr float MAX_DEPTH = 100.0f; // Maximum depth value for the depth buffer

void compute_uv_coords(std::vector<float2>& uvs, int tile_id);
void render_model(uint16_t *buff, float *depth_buf, Resource_manager &manager, Transform &camera, Model &model);

#endif // RENDERER_H