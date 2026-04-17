#ifndef __CONRTOLLER_H__
#define __CONRTOLLER_H__

#include <stdint.h>
#include <stdbool.h>

#include <shader-works/renderer.h>

#include "world.h"

void update_controller(renderer_t *renderer, fps_controller_t *controller, transform_t *camera, bool *mouse_captured, world_t *world);
#endif // __CONRTOLLER_H__
