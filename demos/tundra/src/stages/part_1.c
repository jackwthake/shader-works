#include "stages.h"

#include <stdio.h>
#include <math.h>
#include <SDL3/SDL.h>

#include "../../common/noise.h"
#include "../../common/config.h"

#include "../const.h"
#include "../scene.h"

#define STAGE_01_DISTANCE_TRIGGER 250.0f

// Scene 01: Basic terrain with slight hills moderate trees

float scene_01_terrainHeight(float x, float y, int seed) {
  // Define minimum height for frozen lakes
  const float lake_level = 0.0f;

  // Large-scale gentle rolling terrain (very low frequency for gradual hills)
  float large_hills = fbm(x * 0.004f, y * 0.004f, 4, g_world_config.seed + seed) * 25.0f;

  // Medium-scale terrain features
  float medium_hills = fbm(x * 0.02f, y * 0.02f, 5, g_world_config.seed + seed + 1) * 12.0f;

  // Small-scale detail
  float detail = fbm(x * 0.2f, y * 0.2f, 6, g_world_config.seed + seed + 2) * 4.0f;

  // Domain warping for more interesting mountain shapes
  float warp_x = fbm(x * 0.005f, y * 0.005f, 3, g_world_config.seed + seed + 3) * 20.0f;
  float warp_y = fbm(x * 0.005f, y * 0.005f, 3, g_world_config.seed + seed + 4) * 20.0f;

  // Mountain peaks using warped coordinates
  float mountains = ridgeNoise((x + warp_x) * 0.004f, (y + warp_y) * 0.004f, g_world_config.seed + seed + 5);
  mountains = powf(mountains, 1.5f) * 35.0f; // More dramatic peaks

  // Blend everything together with smooth transitions
  float base_terrain = large_hills + medium_hills * 0.7f + detail * 0.15f;

  float height = base_terrain + 10.f; // + mountain_peaks * mountain_mask * 1.5f;

  // Create frozen lakes by enforcing minimum height
  return fmaxf(height, lake_level);
}

void scene_01_enter(void *args, size_t size) {
  (void)size; // unused
  context_t *ctx = (context_t *)args;

  terrainHeight = scene_01_terrainHeight;

  if (!ctx) return;

  ctx->scene = (scene_t) {
    .camera_pos = {
      .pitch = -0.3f,  // Look down slightly to see the terrain
      .yaw = 0.0f,
      .position = {0, 0, 0}
    },
    .chunk_map = { 0 },
    .controller = (fps_controller_t) {
      .move_speed = 5.0f,
      .mouse_sensitivity = 0.002f,
      .min_pitch = -PI / 2 + EPSILON,
      .max_pitch = PI / 2  - EPSILON,
      .camera_height_offset = 2.0f,
      .delta_time = TICK_INTERVAL,
      .last_frame_time = 0.0f,
      .ground_height = 0.0f
    },
    .sun_color = {90.0f, 110.0f, 180.0f},  // Deep blue
    .fog_color = {162.0f, 208.0f, 227.0},  // Almost white
    .max_tree_chance = 10.0f,
    .fog_start_fasctor = 0.7f
  };

  init_scene(&ctx->scene, g_world_config.max_chunks);

  init_player_camera(ctx);

  // Initialize dog
  init_dog(&ctx->scene.dog, make_float3(5.0f, 0.0f, -15.0f), make_float3(200.0f, 100.0f, 50.0f), 0.6f);
}

void scene_01_tick(void *args, size_t size, float dt) {
  (void)size; // unused
  if (!args) return;

  context_t *ctx = (context_t*)args;

  // apply movement
  apply_fps_movement(ctx, dt);
  ctx->scene.camera_pos.position.y = ctx->scene.controller.ground_height + ctx->scene.controller.camera_height_offset;

  // update snow particles
  update_quads(ctx->scene.camera_pos.position, &ctx->scene.camera_pos);

  // update loaded chunks
  update_loaded_chunks(&ctx->scene);

  // distance from player to dog
  float player_distance = float3_magnitude(float3_sub(ctx->scene.dog.position, ctx->scene.camera_pos.position));

  // update dog
  update_dog(&ctx->scene.dog, player_distance, dt);

  // check for stage change
  if (get_distance_from_origin(ctx->scene.camera_pos.position) > STAGE_01_DISTANCE_TRIGGER) {
    fsm_change_state(ctx->sm, SCENE_02);
    printf("Transitioning to Scene 02\n");
  }
}

int scene_01_render(void *args, size_t size) {
  (void)size; // unused
  if (!args) return 0;

  context_t *ctx = (context_t*)args;

  return render_scene(ctx, ctx->scene.fog_start_fasctor);
}

void scene_01_exit(void *args, size_t size) {
  (void)args; (void)size;
}

float scene_02_height_map(float x, float y, int seed) {
  (void)x; (void)y; (void)seed;
  return 2.0f;
}

void scene_02_enter(void *args, size_t size) {
  (void)size;
  context_t *ctx = (context_t *)args;

  terrainHeight = scene_02_height_map;

  if (!ctx) return;

  ctx->scene = (scene_t) {
    .camera_pos = {
      .pitch = -0.3f,  // Look down slightly to see the terrain
      .yaw = 0.0f,
      .position = {0, 0, 0}
    },
    .chunk_map = { 0 },
    .controller = (fps_controller_t) {
      .move_speed = 15.0f,
      .mouse_sensitivity = 0.002f,
      .min_pitch = -PI / 2 + EPSILON,
      .max_pitch = PI / 2  - EPSILON,
      .camera_height_offset = 3.0f,
      .delta_time = TICK_INTERVAL,
      .last_frame_time = 0.0f,
      .ground_height = 0.0f
    },

    .fog_color = {10.0f, 10.0f, 10.0f}, // night, no trees
    .sun_color = {5.0f, 15.0f, 27.5f},
    .max_tree_chance = 0.0f,
    .fog_start_fasctor = 0.1f
  };

  init_scene(&ctx->scene, g_world_config.max_chunks);

  init_player_camera(ctx);
}

void scene_02_tick(void *args, size_t size, float dt) {
  (void)size; // unused
  if (!args) return;

  context_t *ctx = (context_t*)args;

  // apply movement
  apply_fps_movement(ctx, dt);
  ctx->scene.camera_pos.position.y = ctx->scene.controller.ground_height + ctx->scene.controller.camera_height_offset;

  // update snow particles
  update_quads(ctx->scene.camera_pos.position, &ctx->scene.camera_pos);

  // update loaded chunks
  update_loaded_chunks(&ctx->scene);

  // check for stage change
  // if (get_distance_from_origin(ctx->scene.camera_pos.position) > STAGE_01_DISTANCE_TRIGGER) {
  //   fsm_change_state(ctx->sm, SCENE_02);
  //   printf("Transitioning to Scene 02\n");
  // }
}

int scene_02_render(void *args, size_t size) {
  (void)size; // unused
  if (!args) return 0;

  context_t *ctx = (context_t*)args;

  return render_scene(ctx, ctx->scene.fog_start_fasctor);
}

void scene_02_exit(void *args, size_t size) {
  (void)args; (void)size;
}

// state interface function table for scene 01
state_interface_t scene_01 = {
  .height_map = scene_01_terrainHeight,
  .enter = scene_01_enter,
  .tick = scene_01_tick,
  .render = scene_01_render,
  .exit = scene_01_exit
};

// Scene 02: Desolate flat terrain with sparse trees, night time
state_interface_t scene_02 = {
  .height_map = scene_02_height_map,
  .enter = scene_02_enter,
  .tick = scene_02_tick,
  .render = scene_02_render,
  .exit = scene_02_exit
};