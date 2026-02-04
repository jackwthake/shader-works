#include "stages.h"

#include <stdio.h>
#include <math.h>
#include <SDL3/SDL.h>

#include "../../common/noise.h"
#include "../../common/config.h"

#include "../const.h"
#include "../scene.h"

float get_distance_from_origin(float3 pos) {
  return sqrtf(pos.x * pos.x + pos.z * pos.z);
}

u32 get_sun_color(scene_t *scene) {
  u8 r = (u8)scene->sun_color.x;
  u8 g = (u8)scene->sun_color.y;
  u8 b = (u8)scene->sun_color.z;
  return rgb_to_u32(r, g, b);
}

void get_fog_color(scene_t *scene, u8 *r, u8 *g, u8 *b) {
  *r = (u8)scene->fog_color.x;
  *g = (u8)scene->fog_color.y;
  *b = (u8)scene->fog_color.z;
}

void init_player_camera(context_t *ctx) {
  float terrain_height = get_interpolated_terrain_height(0.0f, 0.0f);
  ctx->scene.controller.ground_height = terrain_height;
  ctx->scene.camera_pos.position.y = terrain_height + ctx->scene.controller.camera_height_offset;

  ctx->renderer.max_depth = MAX_DEPTH;
  ctx->scene.sun = (light_t) {
    .is_directional = true,
    .direction = make_float3(1, -1, 1),
    .color = get_sun_color(&ctx->scene)
  };
  
  ctx->scene.sun.color = get_sun_color(&ctx->scene);

  // Update camera with current position
  update_camera(&ctx->renderer, &ctx->scene.camera_pos);
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

int render_scene(context_t *ctx, float fog_start) {
  usize triangles_rendered = render_loaded_chunks(&ctx->renderer, &ctx->scene, &ctx->scene.sun, 1);
  triangles_rendered += render_quads(&ctx->renderer, &ctx->scene.camera_pos, &ctx->scene.sun, 1);

  u8 fog_r, fog_g, fog_b;
  get_fog_color(&ctx->scene, &fog_r, &fog_g, &fog_b);
  apply_fog_to_screen(&ctx->renderer, ctx->renderer.max_depth * fog_start, ctx->renderer.max_depth - 1.0f, fog_r, fog_g, fog_b);

  return triangles_rendered;
}
