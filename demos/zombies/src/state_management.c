#include <shader-works/primitives.h>
#include <SDL3/SDL.h>

#include <stdbool.h>
#include <stdio.h>

#include "common/state.h"

#include "state_management.h"
#include "controller.h"
#include "world.h"

#define UNUSED(X) (void)(X)

void init_particles(world_t *world, transform_t *cam);
void apply_dust_particles(renderer_t *state, world_t *world, transform_t *cam);

extern renderer_t renderer_state;

state_machine_t game_state = { 0 };
world_t world;

transform_t camera = {0, 0, 0, {0, 2, 0}};
fps_controller_t controller = {
  .move_speed = 8.0f,
  .mouse_sensitivity = 0.0015f,
  .min_pitch = -PI/2 + 0.1f,
  .max_pitch = PI/2 - 0.1f,
  .ground_height = 2.0f,
  .bob_timer = 0.0f,
  .is_moving = false,
  .dash_available = true,
  .dash_timer = 0.0f,
  .dash_active = false,
  .dash_duration = 0.3f,
  .dash_direction = {0, 0, 0},
  .body = {
    .radius = 0.15f,
    .floor_offset = 2.0f,
    .type = TYPE_PLAYER
  }
};

void state_machine_cleanup(void) {
  delete_world(&world);
}

void generate_map_enter(void *state, size_t size) {
  UNUSED(state); UNUSED(size);

  controller.delta_time = SDL_GetPerformanceCounter();
  generate_random_map(&world, MAX_LIGHTS + 10);

  update_camera(&renderer_state, &camera);
  init_particles(&world, &camera);

  world.entity_bodies[MAX_ENTITIES] = &controller.body; // Last body is the player for collision checks
  controller.body.current_sector = point_in_sector(&world, camera.position, NULL); // Put player in the sector corresponding to their starting position
  controller.body.position = camera.position;

  game_state.cleanup = state_machine_cleanup; // Set cleanup function to free world resources when state machine is freed
}

void normal_state_tick(void *state, size_t size, float dt) {
  UNUSED(state); UNUSED(size); UNUSED(dt);
  extern bool mouse_captured;

  update_controller(&renderer_state, &controller, &camera, &mouse_captured, &world);
  update_camera(&renderer_state, &camera);

  tick_entities(&world, &controller, controller.delta_time);

  // IF THERE ARE NO ACTIVE ENEMIES, REGENERATE THE MAP
  bool has_active_enemies = false;
  for (usize i = 0; i < world.num_entities; i++) {
    if (world.entities[i].body.active && world.entities[i].body.type == TYPE_ENEMY) {
      has_active_enemies = true;
      break;
    }
  }

  if (!has_active_enemies) {
    printf("All enemies defeated! Regenerating map...\n");
    fsm_change_state(&game_state, STATE_MAP_REGENERATION);
  }
}

int normal_state_render(void *state, size_t size) {
  UNUSED(state); UNUSED(size);

  render_world(&renderer_state, &world, &camera);
  apply_dust_particles(&renderer_state, &world, &camera);
  return 0;
}

void regenerate_map_enter(void *state, size_t size) {
  UNUSED(state); UNUSED(size);
  printf("Entering map regeneration state...\n");
}

void regenerate_map_tick(void *state, size_t size, float dt) {
  UNUSED(state); UNUSED(size); UNUSED(dt);
  // Map regeneration logic would go here
}

int regenerate_map_render(void *state, size_t size) {
  UNUSED(state); UNUSED(size);
  // Map regeneration rendering would go here (e.g., loading screen)
  return 0;
}

state_interface_t normal_state = {
  .enter = NULL,
  .tick = normal_state_tick,
  .render = normal_state_render,
  .exit = NULL
};

state_interface_t generate_map_state = {
  .enter = generate_map_enter,
  .tick = NULL,
  .render = NULL,
  .exit = NULL
};

state_interface_t regenerate_map_state = {
  .enter = regenerate_map_enter,
  .tick = regenerate_map_tick,
  .render = regenerate_map_render,
  .exit = NULL
};
