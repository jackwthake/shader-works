#ifndef __DEF_H__
#define __DEF_H__

#include <SDL3/SDL.h>
#include <cpu-render/renderer.h>

#define WIN_WIDTH 400
#define WIN_HEIGHT 250
#define WIN_SCALE 4
#define WIN_TITLE "CPU Renderer"

#define ATLAS_WIDTH_PX 8
#define ATLAS_HEIGHT_PX 8

// Fog constants
#define MAX_DEPTH 15.0f
#define FOG_START 5.0f
#define FOG_END   14.5f
#define FOG_R 22
#define FOG_G 35
#define FOG_B 65

// Fixed timestep constants
#define TARGET_TPS 20                      // Aim for 20 ticks per second
#define FIXED_TIMESTEP (1.0 / TARGET_TPS)  // 50ms per tick
#define MAX_FRAME_TIME 0.25                // Cap at 250ms to prevent spiral of death

typedef struct {
  bool running;

  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture* framebuffer_tex;

  // Rendering buffers
  u32 framebuffer[WIN_WIDTH * WIN_HEIGHT];
  f32 depthbuffer[WIN_WIDTH * WIN_HEIGHT];

  renderer_t renderer_state; // from cpu-render/renderer.h
} game_state_t;

#endif /* __DEF_H__ */