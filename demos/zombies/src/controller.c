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
  float running_fov_height = BASE_SCREEN_HEIGHT_WORLD * 1.25f;
  const bool *keys = SDL_GetKeyboardState(NULL);

  float jump_force = 8.0f; // Height of the jump
  float dash_speed = 24.0f; // 3x normal movement speed
  float dash_cooldown = 1.0f; // 1 second cooldown

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

  // Update dash cooldown
  if (!controller->dash_available) {
    controller->dash_timer += controller->delta_time;
    if (controller->dash_timer >= dash_cooldown) {
      controller->dash_available = true;
      controller->dash_timer = 0.0f;
    }
  }

  // Check for dash activation (Shift key)
  if (keys[SDL_SCANCODE_LSHIFT] && controller->dash_available && controller->body.is_grounded) {
    // Calculate horizontal movement direction (ignore vertical)
    float3 dash_dir = make_float3(0, 0, 0);
    bool has_input = false;

    if (keys[SDL_SCANCODE_W]) {
      dash_dir = float3_add(dash_dir, forward);
      has_input = true;
    }
    if (keys[SDL_SCANCODE_S]) {
      dash_dir = float3_add(dash_dir, float3_scale(forward, -1.0f));
      has_input = true;
    }
    if (keys[SDL_SCANCODE_A]) {
      dash_dir = float3_add(dash_dir, right);
      has_input = true;
    }
    if (keys[SDL_SCANCODE_D]) {
      dash_dir = float3_add(dash_dir, float3_scale(right, -1.0f));
      has_input = true;
    }

    // If no directional input, dash forward
    if (!has_input) {
      dash_dir = forward;
    } else {
      // Normalize to prevent diagonal speed boost
      float mag = float3_magnitude(dash_dir);
      if (mag > EPSILON) {
        dash_dir = float3_normalize(dash_dir);
      }
    }

    // Keep only horizontal component for dash
    dash_dir.y = 0;

    controller->dash_active = true;
    controller->dash_timer = 0.0f;
    controller->dash_available = false;
    controller->dash_direction = dash_dir;
  }

  // Update active dash
  if (controller->dash_active) {
    controller->dash_timer += controller->delta_time;
    if (controller->dash_timer >= controller->dash_duration) {
      controller->dash_active = false;
      controller->dash_timer = 0.0f;
    }
  }

  // Apply dash movement if active
  if (controller->dash_active) {
    float dash_distance = dash_speed * controller->delta_time;
    movement = float3_add(movement, float3_scale(controller->dash_direction, dash_distance));
  }

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

  // Determine target FOV based on dash state
  float dash_fov_height = BASE_SCREEN_HEIGHT_WORLD * 1.5f;
  float target_fov_height = running_fov_height;

  if (controller->body.is_grounded) {
    if (controller->is_moving) {
      // Fast bob when walking
      controller->bob_timer += controller->delta_time * 11.0f;
      target_fov_height = controller->dash_active ? dash_fov_height : running_fov_height;
      controller->current_fov_height = lerp(controller->current_fov_height, target_fov_height, controller->delta_time * 5.0f);
      camera->roll = sinf(controller->bob_timer) * 0.01f; // Bob head left and right when moving
    } else {
      // Slow "breathing" when idle
      controller->bob_timer += controller->delta_time * 1.5f;
      target_fov_height = controller->dash_active ? dash_fov_height : BASE_SCREEN_HEIGHT_WORLD;
      controller->current_fov_height = lerp(controller->current_fov_height, target_fov_height, controller->delta_time * 5.0f);
      camera->roll = sinf(controller->bob_timer) * 0.005f; // Very subtle head bob when idle
    }

    float bob_offset = sinf(controller->bob_timer) * 0.12f;
    // We apply the bob offset to the visual height, not the physics position
    float target_eye_level = (float)s->floor_height + 2.0f;
    camera->position.y = lerp(camera->position.y, target_eye_level + bob_offset, controller->delta_time * 5.0f);
  }

  renderer->screen_height_world = controller->current_fov_height;
}
