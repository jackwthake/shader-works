#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include <SDL3/SDL.h>

#include <shader-works/renderer.h>

#include "common/util.h"

#include "state_management.h"
#include "controller.h"
#include "world.h"

#define WIN_WIDTH 320
#define WIN_HEIGHT 155
#define WIN_SCALE 6
#define WIN_TITLE "zombies"
#define MAX_DEPTH 32.f

renderer_t renderer_state = { 0 };
bool mouse_captured = true;

void sys_init(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **framebuffer_tex, bool *mouse_captured) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(WIN_TITLE, WIN_WIDTH * WIN_SCALE, WIN_HEIGHT * WIN_SCALE, 0, window, renderer);

  *framebuffer_tex = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIN_WIDTH, WIN_HEIGHT);
  SDL_SetTextureScaleMode(*framebuffer_tex, SDL_SCALEMODE_NEAREST);

  SDL_SetWindowRelativeMouseMode(*window, true);
  *mouse_captured = true;

  srand(time(NULL));
}

void sdl_consume_events(bool *running, bool *captured, SDL_Window *window) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) *running = false;
    if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.key == SDLK_ESCAPE) {
          *captured = !*captured;
          SDL_SetWindowRelativeMouseMode(window, *captured);
        }
    }
  }
}

void sdl_present(renderer_t *renderer, SDL_Renderer *sdl_renderer, SDL_Texture *framebuffer_tex) {
  SDL_UpdateTexture(framebuffer_tex, NULL, renderer->framebuffer, WIN_WIDTH * sizeof(u32));
  SDL_RenderClear(sdl_renderer);
  SDL_RenderTexture(sdl_renderer, framebuffer_tex, NULL, NULL);
  SDL_RenderPresent(sdl_renderer);
}

int main(int argc, char const *argv[]) {
  UNUSED(argc); UNUSED(argv);
  extern world_t world;
  extern fps_controller_t controller;
  extern transform_t camera;

  SDL_Window *window; SDL_Renderer *renderer; SDL_Texture *fb_tex;
  u32 framebuffer[WIN_WIDTH * WIN_HEIGHT];
  f32 depthbuffer[WIN_WIDTH * WIN_HEIGHT];

  sys_init(&window, &renderer, &fb_tex, &mouse_captured);
  init_renderer(&renderer_state, WIN_WIDTH, WIN_HEIGHT, 0, 0, framebuffer, depthbuffer, NULL, MAX_DEPTH);

  fsm_init(&game_state, STATE_GENERATE_MAP, STATE_NUM_STATES);
  fsm_set_state_interface(&game_state, STATE_GENERATE_MAP, &generate_map_state);
  fsm_set_state_interface(&game_state, STATE_NORMAL, &normal_state);
  fsm_set_state_interface(&game_state, STATE_MAP_REGENERATION, &regenerate_map_state);

  // Load texture atlas before creating world so UVs can be generated
  u32 atlas_w, atlas_h;
  load_bmp_texture("res/base.bmp", &atlas_w, &atlas_h, &renderer_state.texture_atlas);
  renderer_state.atlas_dim.x = (float)atlas_w;
  renderer_state.atlas_dim.y = (float)atlas_h;

  init_world(&world);

  fsm_start(&game_state);
  fsm_change_state(&game_state, STATE_NORMAL);

  bool running = true;
  while (running) {
    uint64_t current_time = SDL_GetPerformanceCounter();
    controller.delta_time = (float)(current_time - controller.last_frame_time) / SDL_GetPerformanceFrequency();
    controller.last_frame_time = current_time;

    sdl_consume_events(&running, &mouse_captured, window);

    if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_R]) {
      fsm_change_state(&game_state, STATE_MAP_REGENERATION);
    }

    // tick world
    fsm_tick_state(&game_state, controller.delta_time);

    // clear framebuffer and depthbuffer
    u32 clear = rgb_to_u32(27, 5, 30);
    for (int i = 0; i < WIN_WIDTH * WIN_HEIGHT; i++) {
      framebuffer[i] = clear;
      depthbuffer[i] = MAX_DEPTH;
    }

    fsm_render_state(&game_state);
    sdl_present(&renderer_state, renderer, fb_tex);
  }

  fsm_free(&game_state);

  SDL_Quit();
  return 0;
}
