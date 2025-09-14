#include <SDL3/SDL.h>

#include "def.h"

struct game_state_t state = {0};


/** Initialize SDL Modules, create window and renderer structs */
static int system_init() {
  SDL_SetAppMetadata("CPU Renderer", "0.0.1", "com.jwt.renderer");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer(WIN_TITLE, WIN_WIDTH * WIN_SCALE, WIN_HEIGHT * WIN_SCALE, 0, &state.window, &state.renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  return SDL_APP_SUCCESS;
}


/** Poll and handle system events */
static void system_poll_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      state.running = false;
    }
  }
}


/** Cleanup and free SDL resources */
static void system_cleanup() {
  if (state.renderer) {
    SDL_DestroyRenderer(state.renderer);
    state.renderer = NULL;
  }

  if (state.window) {
    SDL_DestroyWindow(state.window);
    state.window = NULL;
  }

  SDL_Quit();
}


int main(int argc, char *argv[]) {
  if (system_init() != SDL_APP_SUCCESS) {
    return -1;
  }

  // create framebuffer
  SDL_Texture* framebuffer_tex = SDL_CreateTexture(state.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIN_WIDTH, WIN_HEIGHT);
  SDL_SetTextureScaleMode(framebuffer_tex, SDL_SCALEMODE_NEAREST);

  state.running = true;
  while (state.running) {
    system_poll_events();

    const double now = ((f64)SDL_GetTicks()) / 1000.0;  // convert from milliseconds to seconds.

    // clear the window to the draw color.
    SDL_RenderClear(state.renderer);

    for (i32 y = 0; y < WIN_HEIGHT; y++) {
      for (i32 x = 0; x < WIN_WIDTH; x++) {
        i32 index = y * WIN_WIDTH + x;
        // Simple pattern: moving color bars
        state.framebuffer[index] = 0xFF000000 | ((u32)(x + now * 50) % 256 << 16) | ((u32)(y + now * 50) % 256 << 8) | ((u32)(x + y + now * 50) % 256 << 0);
      }
    }

    // Update framebuffer
    SDL_UpdateTexture(framebuffer_tex, NULL, state.framebuffer, WIN_WIDTH * sizeof(u32));
    SDL_RenderTexture(state.renderer, framebuffer_tex, NULL, NULL);
    SDL_RenderPresent(state.renderer);
  }

  system_cleanup();
  return 0;
}
