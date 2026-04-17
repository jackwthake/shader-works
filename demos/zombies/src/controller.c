#include "controller.h"

#include <math.h>

#include <SDL3/SDL.h>

#include <shader-works/maths.h>
#include <shader-works/renderer.h>

void update_player_sector(transform_t *p, fps_controller_t *controller, world_t *world) {
  sector_t *s = controller->current_sector;

  if (p->position.x < s->x || p->position.x > s->x + s->w || p->position.z < s->z || p->position.z > s->z + s->d) {
    // we exited the current sector, now check the 4 neighbors to find where we went
    for (direction_t dir = 0; dir < 4; dir++) {
        sector_t *n = find_neighbor(world, s, dir);

      if (n) {
          // Is the player inside this neighbor?
        if (p->position.x >= n->x && p->position.x <= n->x + n->w && p->position.z >= n->z && p->position.z <= n->z + n->d) {
          controller->current_sector = n; // Hand-off the new sector to the controller
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

  sector_t *s = controller->current_sector;

  controller->vertical_velocity += gravity * controller->delta_time;
  camera->position.y += controller->vertical_velocity * controller->delta_time;

  if (camera->position.y > s->ceiling_height - 1.0f) {
    camera->position.y = s->ceiling_height - 1.0f;
    controller->vertical_velocity = 0;
  }

  float floor_y = (float)s->floor_height + 2.0f; // Eye level

  if (camera->position.y <= floor_y) {
    camera->position.y = floor_y;
    controller->vertical_velocity = 0;
    controller->is_grounded = true;
  } else {
    controller->is_grounded = false;
  }

  if (keys[SDL_SCANCODE_SPACE] && controller->is_grounded) {
    controller->vertical_velocity = jump_force;
    controller->is_grounded = false;
  }

  float3 pos = camera->position;
  float padding = 0.25f;

  // Horizontal Collision (X)
  float next_x = pos.x + movement.x;
  if (next_x < s->x + padding || next_x > s->x + s->w - padding) {
    direction_t dir = (next_x < s->x + padding) ? DIR_WEST : DIR_EAST;
    sector_t *n = find_neighbor(world, s, dir);
    // Allow if neighbor exists AND isn't a wall (step height check)
    if (n && n->floor_height <= camera->position.y - 0.5f) {
      // Transition potential - check if we actually crossed into the neighbor
      if (next_x < n->x + n->w && next_x > n->x) s = n;
    } else {
      next_x = pos.x; // Blocked
    }
  }
  pos.x = next_x;

  // Vertical Collision (Z)
  float next_z = pos.z + movement.z;
  if (next_z < s->z + padding || next_z > s->z + s->d - padding) {
    direction_t dir = (next_z < s->z + padding) ? DIR_SOUTH : DIR_NORTH;
    sector_t *n = find_neighbor(world, s, dir);
    if (n && n->floor_height <= camera->position.y - 0.5f) {
      if (next_z < n->z + n->d && next_z > n->z) s = n;
    } else {
      next_z = pos.z; // Blocked
    }
  }
  pos.z = next_z;

  // Update State
  camera->position.x = pos.x;
  camera->position.z = pos.z;
  controller->current_sector = s;

  controller->is_moving = (movement.x != 0 || movement.z != 0);
  if (controller->is_grounded) {
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
