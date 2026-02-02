#include "stages.h"

#include <stdio.h>
#include <math.h>
#include <SDL3/SDL.h>

#include "../../common/noise.h"
#include "../../common/config.h"

#include "../const.h"
#include "../scene.h"

#define STAGE_01_DISTANCE_TRIGGER 500.0f

static float get_distance_from_origin(float3 pos) {
  return sqrtf(pos.x * pos.x + pos.z * pos.z);
}

static void get_cycle_color(float time_elapsed, const u8 colors[][3], u8 *r, u8 *g, u8 *b) {
  const float CYCLE_DURATION = 120.0f; // 2 minutes total
  const int NUM_PHASES = 4;

  float cycle_time = fmodf(time_elapsed, CYCLE_DURATION);
  float phase = (cycle_time / CYCLE_DURATION) * NUM_PHASES;

  int idx = (int)phase;
  int next_idx = (idx + 1) % NUM_PHASES;
  float t = phase - idx;

  *r = (u8)lerp(colors[idx][0], colors[next_idx][0], t);
  *g = (u8)lerp(colors[idx][1], colors[next_idx][1], t);
  *b = (u8)lerp(colors[idx][2], colors[next_idx][2], t);
}

u32 get_sun_color(float time_elapsed) {
  u8 r, g, b;
  get_cycle_color(time_elapsed, sun_colors, &r, &g, &b);
  return rgb_to_u32(r, g, b);
}

void get_fog_color(float time_elapsed, u8 *r, u8 *g, u8 *b) {
  get_cycle_color(time_elapsed, fog_colors, r, g, b);
}


void apply_fps_movement(context_t *ctx, float dt) {
  const bool *keys = ctx->keys;
  float3 movement = {0}, right, up, forward;
  float speed = ctx->scene.controller.move_speed * dt;

  // movement
  transform_get_basis_vectors(&ctx->scene.camera_pos, &right, &up, &forward);

  if (keys[SDL_SCANCODE_W]) movement = float3_add(movement, float3_scale(forward, -speed));
  if (keys[SDL_SCANCODE_S]) movement = float3_add(movement, float3_scale(forward, speed));
  if (keys[SDL_SCANCODE_A]) movement = float3_add(movement, float3_scale(right, speed));
  if (keys[SDL_SCANCODE_D]) movement = float3_add(movement, float3_scale(right, -speed));

  // apply movement
  ctx->scene.camera_pos.position = float3_add(ctx->scene.camera_pos.position, movement);
  float new_ground_height = get_interpolated_terrain_height(ctx->scene.camera_pos.position.x, ctx->scene.camera_pos.position.z);

  // mouse input
  float mx, my;
  SDL_GetRelativeMouseState(&mx, &my);

  ctx->scene.camera_pos.yaw += mx * ctx->scene.controller.mouse_sensitivity;
  ctx->scene.camera_pos.pitch -= my * ctx->scene.controller.mouse_sensitivity;

  if (ctx->scene.camera_pos.pitch < ctx->scene.controller.min_pitch) ctx->scene.camera_pos.pitch = ctx->scene.controller.min_pitch;
  if (ctx->scene.camera_pos.pitch > ctx->scene.controller.max_pitch) ctx->scene.camera_pos.pitch = ctx->scene.controller.max_pitch;

  ctx->scene.controller.ground_height = new_ground_height;
  update_camera(&ctx->renderer, &ctx->scene.camera_pos);
}

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
      .move_speed = 15.0f,
      .mouse_sensitivity = 0.002f,
      .min_pitch = -PI / 2 + EPSILON,
      .max_pitch = PI / 2  - EPSILON,
      .camera_height_offset = 3.0f,
      .delta_time = TICK_INTERVAL,
      .last_frame_time = 0.0f,
      .ground_height = 0.0f
    }
  };

  init_scene(&ctx->scene, g_world_config.max_chunks);

  // Set initial camera position and height
  float terrain_height = get_interpolated_terrain_height(0.0f, 0.0f);
  ctx->scene.controller.ground_height = terrain_height;
  ctx->scene.camera_pos.position.y = terrain_height + ctx->scene.controller.camera_height_offset;

  ctx->renderer.max_depth = MAX_DEPTH;
  ctx->scene.sun = (light_t) {
    .is_directional = true,
    .direction = make_float3(1, -1, 1),
    .color = rgb_to_u32(200, 160, 160)
  };

  ctx->renderer.wireframe_mode = false;

  // Update camera with current position
  update_camera(&ctx->renderer, &ctx->scene.camera_pos);
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

  ctx->scene.sun.color = get_sun_color(ctx->total_time);

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

  usize triangles_rendered = render_loaded_chunks(&ctx->renderer, &ctx->scene, &ctx->scene.sun, 1);
  triangles_rendered += render_quads(&ctx->renderer, &ctx->scene.camera_pos, &ctx->scene.sun, 1);

  u8 fog_r, fog_g, fog_b;
  get_fog_color(ctx->total_time, &fog_r, &fog_g, &fog_b);
  apply_fog_to_screen(&ctx->renderer, ctx->renderer.max_depth * 0.95, ctx->renderer.max_depth - 1.0f, fog_r, fog_g, fog_b);

  return triangles_rendered;
}

void scene_01_exit(void *, size_t) {

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
  .height_map = NULL,
  .enter = NULL,
  .tick = NULL,
  .render = NULL,
  .exit = NULL
};