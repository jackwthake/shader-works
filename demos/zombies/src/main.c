#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL3/SDL.h>

#include <shader-works/renderer.h>

#include "controller.h"
#include "world.h"

#define WIN_WIDTH 320
#define WIN_HEIGHT 155
#define WIN_SCALE 6
#define WIN_TITLE "zombies"
#define MAX_DEPTH 64.f

renderer_t renderer_state = { 0 };

void sys_init(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **framebuffer_tex, bool *mouse_captured) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(WIN_TITLE, WIN_WIDTH * WIN_SCALE, WIN_HEIGHT * WIN_SCALE, 0, window, renderer);

  *framebuffer_tex = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIN_WIDTH, WIN_HEIGHT);
  SDL_SetTextureScaleMode(*framebuffer_tex, SDL_SCALEMODE_NEAREST);

  SDL_SetWindowRelativeMouseMode(*window, true);
  *mouse_captured = true;

  srand(time(NULL));
}

void sdl_consume_events(bool *running, bool *mouse_captured, SDL_Window *window) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) *running = false;
    if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.key == SDLK_ESCAPE) {
          *mouse_captured = !*mouse_captured;
          SDL_SetWindowRelativeMouseMode(window, *mouse_captured);
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

  SDL_Window *window; SDL_Renderer *renderer; SDL_Texture *fb_tex;
  u32 framebuffer[WIN_WIDTH * WIN_HEIGHT];
  f32 depthbuffer[WIN_WIDTH * WIN_HEIGHT];
  bool mouse_captured = false;

  world_t world;

  sys_init(&window, &renderer, &fb_tex, &mouse_captured);
  init_renderer(&renderer_state, WIN_WIDTH, WIN_HEIGHT, 0, 0, framebuffer, depthbuffer, NULL, MAX_DEPTH);

  init_world(&world);
  generate_random_map(&world, MAX_LIGHTS + 3);

  transform_t camera = {0, 0, (float3){0, 2, 0}};
  fps_controller_t controller = {
    .move_speed = 8.0f,
    .mouse_sensitivity = 0.0015f,
    .min_pitch = -PI/2 + 0.1f,
    .max_pitch = PI/2 - 0.1f,
    .ground_height = 2.0f,
    .last_frame_time = SDL_GetPerformanceCounter(),
    .bob_timer = 0.0f,
    .is_moving = false,
    .current_sector = world.sectors
  };

  update_camera(&renderer_state, &camera);

  bool running = true;
  while (running) {
    uint64_t current_time = SDL_GetPerformanceCounter();
    controller.delta_time = (float)(current_time - controller.last_frame_time) / SDL_GetPerformanceFrequency();
    controller.last_frame_time = current_time;

    sdl_consume_events(&running, &mouse_captured, window);

    // tick world
    update_controller(&renderer_state, &controller, &camera, &mouse_captured);
    update_player_sector(&camera, &controller, &world);
    update_camera(&renderer_state, &camera);

    // clear framebuffer and depthbuffer
    u32 clear_col = rgb_to_u32(15, 10, 80);
    for (int i = 0; i < WIN_WIDTH * WIN_HEIGHT; i++) {
      framebuffer[i] =  clear_col;
      depthbuffer[i] = MAX_DEPTH;
    }

    render_world(&renderer_state, &world, &camera);
    // apply_fog_to_screen(&renderer_state, 0, MAX_DEPTH / 2, 50, 20, 60);

    sdl_present(&renderer_state, renderer, fb_tex);
    SDL_Delay(1);
  }

  delete_world(&world);
  SDL_Quit();
  return 0;
}
