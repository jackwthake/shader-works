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
          if (p->position.x >= n->x && p->position.x <= n->x + n->w &&
            p->position.z >= n->z && p->position.z <= n->z + n->d) {

            controller->current_sector = n; // Hand-off the new sector to the controller
            return;
          }
        }
      }

      // If we get here, the player walked into the "Void" (no neighbor)
  }
}

void update_controller(renderer_t *renderer, fps_controller_t *controller, transform_t *camera, world_t *world, bool *mouse_captured) {
  float running_fov_height = BASE_SCREEN_HEIGHT_WORLD * 1.5f;
  const bool *keys = SDL_GetKeyboardState(NULL);

  // Mouse input
  if (*mouse_captured) {
    float mx, my;
    SDL_GetRelativeMouseState(&mx, &my);
    camera->yaw += mx * controller->mouse_sensitivity;
    camera->pitch -= my * controller->mouse_sensitivity;
    if (camera->pitch < controller->min_pitch) camera->pitch = controller->min_pitch;
    if (camera->pitch > controller->max_pitch) camera->pitch = controller->max_pitch;
  }

  // Keyboard movement
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

  camera->position = float3_add(camera->position, movement);
  controller->is_moving = (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_S] ||
                           keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_D]);

  update_player_sector(camera, controller, world);

  if (controller->is_moving) {
    controller->bob_timer += controller->delta_time * 11.0f;
    controller->current_fov_height = lerp(controller->current_fov_height, running_fov_height, controller->delta_time * 5.0f);
  } else {
    controller->bob_timer += controller->delta_time * 2.0f;
    controller->current_fov_height = lerp(controller->current_fov_height, BASE_SCREEN_HEIGHT_WORLD, controller->delta_time * 5.0f);
  }

  float bob_amount = 0.15f;
  float bob_offset = sinf(controller->bob_timer) * bob_amount;

  controller->ground_height = controller->current_sector->floor_height + 2.f;

  // Apply bob to vertical position
  camera->position.y = controller->ground_height + bob_offset;
  renderer->screen_height_world = controller->current_fov_height;
}
