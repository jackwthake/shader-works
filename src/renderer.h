#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <stdint.h>

#include "util/maths.h"
#include "util/model.h"

void compute_uv_coords(std::vector<float2>& uvs, int tile_id);
void render_model(uint16_t *buff, Transform &camera, Model &model);

#endif // RENDERER_H