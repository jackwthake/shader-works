#include "controller.h"

#include <math.h>

#include <SDL3/SDL.h>

#include <shader-works/maths.h>
#include <shader-works/renderer.h>

void update_controller(renderer_t *renderer, fps_controller_t *controller, transform_t *camera, bool *mouse_captured) {
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

  if (controller->is_moving) {
    controller->bob_timer += controller->delta_time * 11.0f;
    controller->current_fov_height = lerp(controller->current_fov_height, running_fov_height, controller->delta_time * 5.0f);
  } else {
    controller->bob_timer += controller->delta_time * 2.0f;
    controller->current_fov_height = lerp(controller->current_fov_height, BASE_SCREEN_HEIGHT_WORLD, controller->delta_time * 5.0f);
  }

  float bob_amount = 0.15f;
  float bob_offset = sinf(controller->bob_timer) * bob_amount;

  // Apply bob to vertical position
  camera->position.y = controller->ground_height + bob_offset;
  renderer->screen_height_world = controller->current_fov_height;
}
