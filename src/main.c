#include <SDL3/SDL.h>
#include <assert.h>
#include <float.h>
#include <math.h>

#include "def.h"
#include "renderer.h"
#include "maths.h"

// Fixed timestep constants
#define TARGET_TPS 20
#define FIXED_TIMESTEP (1.0 / TARGET_TPS)  // 50ms per tick
#define MAX_FRAME_TIME 0.25                // Cap at 250ms to prevent spiral of death

// Initialize SDL Modules, create window and renderer structs
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

// Update game logic with fixed timestep 
static void update_game(game_state_t* state, transform_t* camera, model_t* cube_model, f64 dt) {
  // dt will always be FIXED_TIMESTEP
  
  cube_model->transform.yaw += 1.0f * dt;
  cube_model->transform.pitch += 0.5f * dt;
  cube_model->transform.position.z = 5.0f + sinf(SDL_GetTicks() / 1000.0f) * 2.0f; // Move cube in and out
  
  // Update camera matrices
  update_camera(state, camera);
}

// Render the current frame
static void render_frame(game_state_t* state, transform_t* camera, model_t* cube_model) {
  // Clear framebuffer and reset depth buffer
  for(int i = 0; i < WIN_WIDTH * WIN_HEIGHT; ++i) {
    state->framebuffer[i] = rgb_to_888(FOG_R, FOG_G, FOG_B); // Clear to fog color
    state->depthbuffer[i] = FLT_MAX;
  }

  render_model(state, camera, cube_model, default_shader, NULL, 0);

  apply_fog_to_screen(state);
  // Update SDL texture and present
  SDL_UpdateTexture(state->framebuffer_tex, NULL, state->framebuffer, WIN_WIDTH * sizeof(u32));
  SDL_RenderTexture(state->renderer, state->framebuffer_tex, NULL, NULL);
  SDL_RenderPresent(state->renderer);
}

// Cleanup and free SDL resources on exit
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

  // Initialize depth buffer to maximum depth
  for (int i = 0; i < WIN_WIDTH * WIN_HEIGHT; ++i) {
    state.depthbuffer[i] = FLT_MAX;
  }

  if (system_init(&state) != SDL_APP_SUCCESS) {
    return -1;
  }

  // Create framebuffer texture
  state.framebuffer_tex = SDL_CreateTexture(state.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIN_WIDTH, WIN_HEIGHT);
  SDL_SetTextureScaleMode(state.framebuffer_tex, SDL_SCALEMODE_NEAREST);
  assert(state.framebuffer_tex != NULL);

  init_renderer(&state);

  // Initialize game objects
  model_t cube_model = {0};
  cube_model.vertices = cube_vertices;
  cube_model.num_vertices = CUBE_VERTEX_COUNT;
  cube_model.scale = make_float3(1.0f, 1.0f, 1.0f);
  cube_model.transform.position = make_float3(0.0f, 0.0f, -5.0f);

  transform_t camera = {0};
  camera.position = make_float3(0.0f, 0.0f, 0.0f);
  camera.yaw = 0.0f;
  camera.pitch = 0.0f;

  update_camera(&state, &camera);
  state.use_textures = false;
  state.running = true;

  // Fixed timestep loop variables
  f64 current_time = SDL_GetTicks() / 1000.0;
  f64 accumulator = 0.0;
  
  // Performance tracking (optional)
  int frame_count = 0;
  f64 fps_timer = current_time;

  while (state.running) {
    f64 new_time = SDL_GetTicks() / 1000.0;
    f64 frame_time = new_time - current_time;
    current_time = new_time;

    // Prevent spiral of death - cap frame time
    frame_time = fmin(frame_time, MAX_FRAME_TIME);
    accumulator += frame_time;

    // Process input (this can happen at variable rate)
    system_poll_events(&state);

    // Fixed timestep updates
    while (accumulator >= FIXED_TIMESTEP) {
      update_game(&state, &camera, &cube_model, FIXED_TIMESTEP);
      accumulator -= FIXED_TIMESTEP;
    }
    
    SDL_RenderClear(state.renderer);
    render_frame(&state, &camera, &cube_model);

    // FPS counter
    frame_count++;
    if (current_time - fps_timer >= 1.0) {
      SDL_Log("FPS: %d", frame_count);
      frame_count = 0;
      fps_timer = current_time;
    }

    SDL_Delay(1); // Small delay to prevent 100% CPU usage
  }

  SDL_DestroyTexture(state.framebuffer_tex);
  system_cleanup(&state);
  return 0;
}