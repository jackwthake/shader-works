#include <SDL3/SDL.h>

#include "def.h"

#define WIN_WIDTH 480
#define WIN_HEIGHT 240
#define WIN_SCALE 4
const unsigned char *WIN_TITLE = "CPU Renderer";

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
u32 framebuffer[WIN_WIDTH * WIN_HEIGHT] = {0};

int main(int argc, char *argv[]) {
  bool running = true;
  SDL_SetAppMetadata("CPU Renderer", "0.0.1", "com.jwt.renderer");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer(WIN_TITLE, WIN_WIDTH * WIN_SCALE, WIN_HEIGHT * WIN_SCALE, 0, &window, &renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  // create framebuffer
  SDL_Texture* framebuffer_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIN_WIDTH, WIN_HEIGHT);
  SDL_SetTextureScaleMode(framebuffer_tex, SDL_SCALEMODE_PIXELART);

  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }

    const double now = ((double)SDL_GetTicks()) / 1000.0;  // convert from milliseconds to seconds.

    // clear the window to the draw color.
    SDL_RenderClear(renderer);

    for (int y = 0; y < WIN_HEIGHT; y++) {
      for (int x = 0; x < WIN_WIDTH; x++) {
        int index = y * WIN_WIDTH + x;
        // Simple pattern: moving color bars
        framebuffer[index] = 0xFF000000 | ((u32)(x + now * 50) % 256 << 16) | ((u32)(y + now * 50) % 256 << 8) | ((u32)(x + y + now * 50) % 256 << 0);
      }
    }

    // Update framebuffer
    SDL_UpdateTexture(framebuffer_tex, NULL, framebuffer, WIN_WIDTH * sizeof(u32));
    SDL_RenderTexture(renderer, framebuffer_tex, NULL, NULL);
    SDL_RenderPresent(renderer);
  }


  // Cleanup
  if (renderer) {
    SDL_DestroyRenderer(renderer);
    renderer = NULL;
  }

  if (window) {
    SDL_DestroyWindow(window);
    window = NULL;
  }

  SDL_Quit();
  return 0;
}
