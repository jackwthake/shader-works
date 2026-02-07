#include <shader-works/renderer.h>

#include <float.h> // FLT_MAX
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>

#include "common/config.h"
#include "common/noise.h"

#include "stages/stages.h"
#include "const.h"

#define NUM_SCENES 9
state_interface_t *scenes[NUM_SCENES] = {
    &scene_menu,
    &scene_01,
    &scene_02,
    &scene_03,
    &scene_04,
    &scene_05,
    &scene_06,
    &scene_07,
    &scene_08,
};

static void SDL_library_init(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **frame_buf, const char *title, int width, int height, int scale) {
  SDL_Init(SDL_INIT_VIDEO);

  SDL_CreateWindowAndRenderer(title, width * scale, height * scale, 0, window, renderer);

  *frame_buf = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
  SDL_SetTextureScaleMode(*frame_buf, SDL_SCALEMODE_NEAREST);

  SDL_SetWindowRelativeMouseMode(*window, false);
}

static void init_performance_counter(performance_counter *stats) {
  stats->fps_counter = 0;
  stats->tps_counter = 0;
  stats->triangle_counter = 0;
  stats->last_counter_time = SDL_GetPerformanceCounter();
}

static void init_state_machine(state_machine_t *sm, context_t *state_context) {
  fsm_init(sm, SCENE_01, NUM_SCENES);

  // link state interfaces
  // fsm_set_state_interface(sm, SCENE_MENU, &scene_menu);
  fsm_set_state_interface(sm, SCENE_01, &scene_01);
  fsm_set_state_interface(sm, SCENE_02, &scene_02);
  // fsm_set_state_interface(sm, SCENE_03, &scene_03);
  // fsm_set_state_interface(sm, SCENE_04, &scene_04);
  // fsm_set_state_interface(sm, SCENE_05, &scene_05);
  // fsm_set_state_interface(sm, SCENE_06, &scene_06);
  // fsm_set_state_interface(sm, SCENE_07, &scene_07);
  // fsm_set_state_interface(sm, SCENE_08, &scene_08);

  fsm_update_internal_state(sm, state_context, sizeof(context_t));
}

int main(int argc, char const *argv[]) {
  (void)argc; (void)argv;

  // Load window configuration from config.json
  unsigned int config_width, config_height, config_scale;
  char config_title[256];

  load_config(&config_width, &config_height, &config_scale, config_title, sizeof(config_title));
  load_world_config();

  SDL_Window *sdl_window;
  SDL_Renderer *sdl_renderer;
  SDL_Texture *sdl_framebuff;

  u32 *framebuffer = (u32 *)malloc(config_width * config_height * sizeof(u32));
  f32 *depth_buffer = (f32 *)malloc(config_width * config_height * sizeof(f32));

  // Initialize state and window
  SDL_library_init(&sdl_window, &sdl_renderer, &sdl_framebuff, config_title, config_width, config_height, config_scale);
  SDL_SetWindowRelativeMouseMode(sdl_window, true);

  renderer_t renderer = {0};
  init_renderer(&renderer, config_width, config_height, 0, 0, framebuffer, depth_buffer, MAX_DEPTH);

  performance_counter stats;
  init_performance_counter(&stats);

  // Initialize state machine
  state_machine_t sm = {0};

  context_t state_context = {
    .framebuffer = framebuffer,
    .depth_buffer = depth_buffer,
    .renderer = renderer,

    .keys = SDL_GetKeyboardState(NULL),
    .sm = &sm,
    .total_time = 0.0f
  };

  init_state_machine(&sm, &state_context);
  fsm_start(&sm);

  bool running = true;
  float accumulator = 0.0f;
  uint64_t last_time = SDL_GetPerformanceCounter();

  while (running) {
    uint64_t current_time = SDL_GetPerformanceCounter();
    float frame_time = (float)(current_time - last_time) / (float)SDL_GetPerformanceFrequency();
    last_time = current_time;

    // Cap frame time to prevent spiral of death
    if (frame_time > 0.1f) frame_time = 0.1f;

    accumulator += frame_time;
    state_context.total_time += frame_time;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) running = false;
      if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.key == SDLK_ESCAPE) {
          running = false;
        }
      }
    }

    // Fixed timestep game updates
    while (accumulator >= TICK_INTERVAL) {
      fsm_tick_state(&sm, TICK_INTERVAL);

      accumulator -= TICK_INTERVAL;
      stats.tps_counter++;
    }

    u8 bg_r, bg_g, bg_b;
    get_fog_color(&state_context.scene, &bg_r, &bg_g, &bg_b);
    u32 background_color = rgb_to_u32(bg_r, bg_g, bg_b);

    for(int i = 0; i < config_width * config_height; ++i) {
      framebuffer[i] = background_color;
      depth_buffer[i] = FLT_MAX;
    }

    int triangles_rendered = fsm_render_state(&sm);

    SDL_UpdateTexture(sdl_framebuff, NULL, framebuffer, config_width * sizeof(u32));
    SDL_RenderTexture(sdl_renderer, sdl_framebuff, NULL, NULL);
    SDL_RenderPresent(sdl_renderer);

    stats.fps_counter++;
    stats.triangle_counter += triangles_rendered;
    uint64_t counter_time = SDL_GetPerformanceCounter();
    if ((float)(counter_time - stats.last_counter_time) / (float)SDL_GetPerformanceFrequency() >= 1.0f) {
      uint64_t avg_triangles_per_frame = stats.fps_counter > 0 ? stats.triangle_counter / stats.fps_counter : 0;
      printf("TPS: %lu, FPS: %lu, Triangles/frame: %lu, Player: (%.1f, %.1f, %.1f), Dog Active? %d, Dog Position (%.1f, %.1f, %.1f)\n",
              stats.tps_counter, stats.fps_counter, avg_triangles_per_frame,
              state_context.scene.camera_pos.position.x, state_context.scene.camera_pos.position.y, state_context.scene.camera_pos.position.z, state_context.scene.dog.active, state_context.scene.dog.active ? state_context.scene.dog.position.x : 0.0f, state_context.scene.dog.active ? state_context.scene.dog.position.y : 0.0f, state_context.scene.dog.active ? state_context.scene.dog.position.z : 0.0f);
      stats.tps_counter = 0;
      stats.fps_counter = 0;
      stats.triangle_counter = 0;
      stats.last_counter_time = counter_time;
    }
  }

  free_chunk_map(&state_context.scene.chunk_map);
  fsm_free(&sm);

  free(framebuffer);
  free(depth_buffer);

  SDL_DestroyTexture(sdl_framebuff);
  SDL_DestroyRenderer(sdl_renderer);
  SDL_DestroyWindow(sdl_window);
  SDL_Quit();

  free_config();

  return 0;
}
