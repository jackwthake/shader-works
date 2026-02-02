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

// Day/night cycle color keyframes: Dawn -> Noon -> Dusk -> Midnight
static const u8 sun_colors[][3] = {
  {30, 50, 120},    // Dawn: deep blue
  {200, 160, 160},  // Noon: white
  {255, 100, 150},  // Dusk: light pink
  {20, 20, 100},    // Midnight: dark blue
};

static const u8 fog_colors[][3] = {
  {20, 30, 80},     // Dawn: rich deep blue
  {240, 245, 250},  // Noon: almost white
  {255, 150, 80},   // Dusk: pastel orange
  {0, 0, 0},        // Midnight: black
};

void get_fog_color(float time_elapsed, u8 *r, u8 *g, u8 *b);
u32 get_sun_color(float time_elapsed);

#endif // __CONST_H__