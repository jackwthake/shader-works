#ifndef __CONST_H__
#define __CONST_H__

#include <stdint.h>
#include <shader-works/maths.h>
#include <shader-works/renderer.h>
#include "util/state.h"
#include "scene.h"

// Default values
#define MAX_DEPTH 95.0f // 3 chunks - 1

typedef enum {
  GENERATE,
  NORMAL,
  OVERHEAD,
  NUM_STATES
} state;

typedef struct {
  uint64_t fps_counter;
  uint64_t tps_counter;
  uint64_t triangle_counter;
  uint64_t last_counter_time;
} performance_counter;

typedef struct {
  u32 *framebuffer;
  f32 *depth_buffer;

  renderer_t renderer;
  scene_t scene;
  const bool *keys;

  float total_time;

  state_machine_t *sm;
} context_t;

static const float TICK_RATE = 20.0f; // 20 TPS
static const float TICK_INTERVAL = 1.0f / TICK_RATE;

void get_fog_color(scene_t *scene, u8 *r, u8 *g, u8 *b);
u32 get_sun_color(scene_t *scene);

#endif // __CONST_H__