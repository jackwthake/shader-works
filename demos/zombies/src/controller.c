#include "controller.h"

#include <math.h>

#include <SDL3/SDL.h>

#include <shader-works/maths.h>
#include <shader-works/renderer.h>

void update_player_sector(transform_t *p, fps_controller_t *controller, world_t *world) {
  sector_t *s = controller->body.current_sector;

  if (p->position.x < s->x || p->position.x > s->x + s->w || p->position.z < s->z || p->position.z > s->z + s->d) {
    // we exited the current sector, now check the 4 neighbors to find where we went
    for (direction_t dir = 0; dir < 4; dir++) {
        sector_t *n = find_neighbor(world, s, dir);

      if (n) {
          // Is the player inside this neighbor?
        if (p->position.x >= n->x && p->position.x <= n->x + n->w && p->position.z >= n->z && p->position.z <= n->z + n->d) {
          controller->body.current_sector = n; // Hand-off the new sector to the controller
          return;
        }
      }
    }
  }
}

void update_controller(renderer_t *renderer, fps_controller_t *controller, transform_t *camera, bool *mouse_captured, world_t *world) {
  float running_fov_height = BASE_SCREEN_HEIGHT_WORLD * 1.5f;
  const bool *keys = SDL_GetKeyboardState(NULL);

  float gravity = -15.5f; // Strength of gravity
  float jump_force = 8.0f; // Height of the jump

  if (*mouse_captured) {
    float mx, my;
    SDL_GetRelativeMouseState(&mx, &my);
    camera->yaw += mx * controller->mouse_sensitivity;
    camera->pitch -= my * controller->mouse_sensitivity;
    camera->pitch = fmaxf(controller->min_pitch, fminf(camera->pitch, controller->max_pitch));
  }

  float cy = cosf(camera->yaw), sy = sinf(camera->yaw);
  float cp = cosf(camera->pitch), sp = sinf(camera->pitch);
  float3 right = make_float3(cy, 0, -sy);
  float3 forward = make_float3(-sy * cp, sp, -cy * cp);
  float3 movement = make_float3(0, 0, 0);
  float speed = controller->move_speed * controller->delta_time;

  if (keys[SDL_SCANCODE_W]) movement = float3_add(movement, float3_scale(forward, speed));
  if (keys[SDL_SCANCODE_S]) movement = float3_add(movement, float3_scale(forward, -speed));
  if (keys[SDL_SCANCODE_A]) movement = float3_add(movement, float3_scale(right, speed));
  if (keys[SDL_SCANCODE_D]) movement = float3_add(movement, float3_scale(right, -speed));

  sector_t *s = controller->body.current_sector;

  if (controller->body.position.y > s->ceiling_height - 1.0f) {
    controller->body.position.y = s->ceiling_height - 1.0f;
    controller->body.y_velocity = 0;
  }

  float floor_y = (float)s->floor_height + controller->body.floor_offset; // Eye level

  if (controller->body.position.y <= floor_y) {
    controller->body.position.y = floor_y;
    controller->body.y_velocity = 0;
    controller->body.is_grounded = true;
  } else {
    controller->body.is_grounded = false;
  }

  if (keys[SDL_SCANCODE_SPACE] && controller->body.is_grounded) {
    controller->body.y_velocity = jump_force;
    controller->body.is_grounded = false;
  }

  resolve_world_collision(world, &controller->body, movement, controller->delta_time);
  camera->position = controller->body.position;
  update_player_sector(camera, controller, world);

  controller->is_moving = (movement.x != 0 || movement.z != 0);
  if (controller->body.is_grounded) {
    if (controller->is_moving) {
      // Fast bob when walking
      controller->bob_timer += controller->delta_time * 11.0f;
      controller->current_fov_height = lerp(controller->current_fov_height, running_fov_height, controller->delta_time * 5.0f);
    } else {
      // Slow "breathing" when idle
      controller->bob_timer += controller->delta_time * 2.0f;
      controller->current_fov_height = lerp(controller->current_fov_height, BASE_SCREEN_HEIGHT_WORLD, controller->delta_time * 5.0f);
    }

    float bob_offset = sinf(controller->bob_timer) * 0.12f;
    // We apply the bob offset to the visual height, not the physics position
    float target_eye_level = (float)s->floor_height + 2.0f;
    camera->position.y = lerp(camera->position.y, target_eye_level + bob_offset, controller->delta_time * 5.0f);
  }

  renderer->screen_height_world = controller->current_fov_height;
}
