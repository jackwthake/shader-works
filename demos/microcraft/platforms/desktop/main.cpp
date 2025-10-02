#include <float.h>
#include <math.h>
#include <stdbool.h>

extern "C" {
#include <shader-works/renderer.h>
#include <shader-works/primitives.h>
#include <shader-works/shaders.h>
#include <shader-works/maths.h>
}

#include <SDL3/SDL.h>

#include "scene.hpp"
#include "resources.inl"

#define WIN_WIDTH 400
#define WIN_HEIGHT 250
#define WIN_SCALE 4
#define WIN_TITLE "MicroCraft"
#define MAX_DEPTH 15

// Required pixel conversion fucntions
u32 rgb_to_u32(u8 r, u8 g, u8 b) {
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);
  return SDL_MapRGBA(format, NULL, r, g, b, 255);
}

void u32_to_rgb(u32 color, u8 *r, u8 *g, u8 *b) {
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);
  SDL_GetRGB(color, format, NULL, r, g, b);
}

// Update fps controller state timing - called by game code
void update_timing(fps_controller_t &controller) {
  uint64_t current_time = SDL_GetPerformanceCounter();
  controller.delta_time = (float)(current_time - controller.last_frame_time) / SDL_GetPerformanceFrequency();
  controller.last_frame_time = current_time;
  if (controller.delta_time > 0.1f) controller.delta_time = 0.1f;
}

// update controller based off keyboard input - called by game code
void handle_input(fps_controller_t &controller, transform_t &camera) {
  const bool *keys = SDL_GetKeyboardState(NULL);

  // Mouse input
  float mx, my;
  SDL_GetRelativeMouseState(&mx, &my);
  camera.yaw += mx * controller.mouse_sensitivity;
  camera.pitch -= my * controller.mouse_sensitivity;
  if (camera.pitch < controller.min_pitch) camera.pitch = controller.min_pitch;
  if (camera.pitch > controller.max_pitch) camera.pitch = controller.max_pitch;

  // Keyboard movement
  float cy = cosf(camera.yaw), sy = sinf(camera.yaw);
  float cp = cosf(camera.pitch), sp = sinf(camera.pitch);
  float3 right = make_float3(cy, 0, -sy);
  float3 forward = make_float3(-sy * cp, sp, -cy * cp);
  float3 movement = make_float3(0, 0, 0);
  float speed = controller.move_speed * controller.delta_time;

  if (keys[SDL_SCANCODE_W]) movement = float3_add(movement, float3_scale(forward, speed));
  if (keys[SDL_SCANCODE_S]) movement = float3_add(movement, float3_scale(forward, -speed));
  if (keys[SDL_SCANCODE_A]) movement = float3_add(movement, float3_scale(right, speed));
  if (keys[SDL_SCANCODE_D]) movement = float3_add(movement, float3_scale(right, -speed));

  camera.position = float3_add(camera.position, movement);
  camera.position.y = controller.ground_height;
}

int main(int argc, char *argv[]) {
  SDL_Init(SDL_INIT_VIDEO);

  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_CreateWindowAndRenderer(WIN_TITLE, WIN_WIDTH * WIN_SCALE, WIN_HEIGHT * WIN_SCALE, 0, &window, &renderer);

  SDL_Texture *framebuffer_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIN_WIDTH, WIN_HEIGHT);
  SDL_SetTextureScaleMode(framebuffer_tex, SDL_SCALEMODE_NEAREST);

  u32 framebuffer[WIN_WIDTH * WIN_HEIGHT];
  f32 depthbuffer[WIN_WIDTH * WIN_HEIGHT];

  SDL_SetWindowRelativeMouseMode(window, true);

  renderer_t renderer_state = {0};
  // Atlas dimensions: 10 tiles x 3 rows, each tile is 8x8 pixels
  init_renderer(&renderer_state, WIN_WIDTH, WIN_HEIGHT, 80, 24, framebuffer, depthbuffer, MAX_DEPTH);

  renderer_state.texture_atlas = files[1].data;

  Scene scene;

  scene.init();

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) running = false;
      if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) running = false;
    }

    scene.update(0.05f);

    for(int i = 0; i < WIN_WIDTH * WIN_HEIGHT; ++i) {
      framebuffer[i] = rgb_to_u32(50, 50, 175);
      depthbuffer[i] = FLT_MAX;
    }

    scene.render(renderer_state, framebuffer, depthbuffer);

    SDL_UpdateTexture(framebuffer_tex, NULL, framebuffer, WIN_WIDTH * sizeof(u32));
    SDL_RenderTexture(renderer, framebuffer_tex, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyTexture(framebuffer_tex);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
