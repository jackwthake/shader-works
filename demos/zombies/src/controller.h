#ifndef __CONRTOLLER_H__
#define __CONRTOLLER_H__

#include <stdint.h>
#include <stdbool.h>

#include <shader-works/renderer.h>

#include "world.h"

typedef struct {
  float move_speed;
  float mouse_sensitivity;
  float min_pitch, max_pitch;
  float ground_height;
  float delta_time;
  uint64_t last_frame_time;
  float bob_timer;
  bool is_moving;
  float current_fov_height;

  sector_t *current_sector;
} fps_controller_t;

void update_controller(renderer_t *renderer, fps_controller_t *controller, transform_t *camera, world_t *world, bool *mouse_captured);

#endif // __CONRTOLLER_H__
