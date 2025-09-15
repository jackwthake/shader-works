#include <SDL3/SDL.h>
#include <assert.h>
#include <float.h>

#include "def.h"
#include "renderer.h"
#include "maths.h"

/** Initialize SDL Modules, create window and renderer structs */
static int system_init(game_state_t* state) {
  assert(state != NULL);

  SDL_SetAppMetadata("CPU Renderer", "0.0.1", "com.jwt.renderer");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer(WIN_TITLE, WIN_WIDTH * WIN_SCALE, WIN_HEIGHT * WIN_SCALE, 0, &state->window, &state->renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  return SDL_APP_SUCCESS;
}

/** Poll and handle system events */
static void system_poll_events(game_state_t* state) {
  assert(state != NULL);
  assert(state->running);

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      state->running = false;
    }
  }
}

/** Cleanup and free SDL resources */
static void system_cleanup(game_state_t* state) {
  assert(state != NULL);

  if (state->renderer) {
    SDL_DestroyRenderer(state->renderer);
    state->renderer = NULL;
  }

  if (state->window) {
    SDL_DestroyWindow(state->window);
    state->window = NULL;
  }

  SDL_Quit();
}


int main(int argc, char *argv[]) {
  game_state_t state = {0};

  // Initialize depth buffer to maximum depth (FLT_MAX)
  for (int i = 0; i < WIN_WIDTH * WIN_HEIGHT; ++i) {
    state.depthbuffer[i] = FLT_MAX;
  }

  if (system_init(&state) != SDL_APP_SUCCESS) {
    return -1;
  }

  // create framebuffer
  SDL_Texture* framebuffer_tex = SDL_CreateTexture(state.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIN_WIDTH, WIN_HEIGHT);
  SDL_SetTextureScaleMode(framebuffer_tex, SDL_SCALEMODE_NEAREST);

  assert(framebuffer_tex != NULL);
  init_renderer(&state);

  model_t cube_model = {0};
  cube_model.vertices = cube_vertices;
  cube_model.num_vertices = CUBE_VERTEX_COUNT;
  cube_model.scale = make_float3(1.0f, 1.0f, 1.0f);
  cube_model.transform.position = make_float3(0.0f, 0.0f, 5.0f);

  transform_t camera = {0};
  camera.position = make_float3(0.0f, 0.0f, 0.0f);
  camera.yaw = 0.0f;
  camera.pitch = 0.0f;

  update_camera(&state, &camera);

  state.use_textures = false;
  state.running = true;
  while (state.running) {
    system_poll_events(&state);

    const f64 now = ((f64)SDL_GetTicks()) / 1000.0;  // convert from milliseconds to seconds.

    cube_model.transform.position = make_float3(0.0f, 0.0f, 5.0f + sinf((f32)now) * 2.0f);
    cube_model.transform.yaw = (f32)now * 0.5f;
    cube_model.transform.pitch = (f32)now * 0.3f;

    // clear the window to the draw color.
    SDL_RenderClear(state.renderer);

    // Clear framebuffer and reset depth buffer each frame
    for(int i = 0; i < WIN_WIDTH * WIN_HEIGHT; ++i) {
      state.framebuffer[i] = FOG_COLOR;
      state.depthbuffer[i] = FLT_MAX;  // Reset depth buffer each frame
    }

    update_camera(&state, &camera);
    render_model(&state, &camera, &cube_model, default_shader, NULL, 0);

    // Update framebuffer
    SDL_UpdateTexture(framebuffer_tex, NULL, state.framebuffer, WIN_WIDTH * sizeof(u32));
    SDL_RenderTexture(state.renderer, framebuffer_tex, NULL, NULL);
    SDL_RenderPresent(state.renderer);
  }

  system_cleanup(&state);
  return 0;
}