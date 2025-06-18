#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <stdint.h>

#include "util/maths.h"
#include "util/model.h"

void render_model(uint16_t * &buff, Model &model);

#endif // RENDERER_H