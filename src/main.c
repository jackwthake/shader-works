#include <SDL3/SDL.h>

#include <stdlib.h>
#include <assert.h>
#include <float.h>
#include <math.h>

#include "def.h"
#include "renderer.h"
#include "maths.h"

#include "resources.inl"

// Fixed timestep constants
#define TARGET_TPS 20                      // Aim for 20 ticks per second
#define FIXED_TIMESTEP (1.0 / TARGET_TPS)  // 50ms per tick
#define MAX_FRAME_TIME 0.25                // Cap at 250ms to prevent spiral of death

// strobing color
static u32 plane_frag_shader_func(u32 input_color, void *args, usize argc) {
  // Get current time in seconds
  f32 time = SDL_GetTicks() / 1000.0f;
  
  // Create waves based on time
  f32 wave1 = sinf(time * 2.0f) * 0.5f + 0.5f;          // Slow wave
  f32 wave2 = sinf(time * 5.0f) * 0.3f + 0.5f;          // Fast wave
  f32 wave3 = cosf(time * 3.0f) * 0.4f + 0.6f;          // Medium wave
  
  // Create animated colors
  u8 r = (u8)(wave1 * 255.0f * wave3);
  u8 g = (u8)(wave2 * 180.0f * wave1);
  u8 b = (u8)((1.0f - wave1) * 220.0f * wave2);
  
  return rgb_to_888(r, g, b);
}

// Initialize SDL Modules, create window and renderer structs
static int system_init(game_state_t* state) {
  assert(state != NULL);
  
  SDL_SetAppMetadata(WIN_TITLE, "0.0.1", WIN_TITLE);
  
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

// Poll and handle system events
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
static void update_game(game_state_t* state, transform_t* camera, model_t* cube_model, model_t* plane_model, f64 dt) {
  // dt will always be FIXED_TIMESTEP
  
  cube_model->transform.yaw += 1.0f * dt;
  cube_model->transform.pitch += 0.5f * dt;
  cube_model->transform.position.z = 5.0f + sinf(SDL_GetTicks() / 1000.0f) * 2.0f;

  // wripple plane vertices based off of time
  for (int i = 0; i < plane_model->num_vertices; ++i) {
    plane_model->vertices[i].y = (0.5f + sinf(SDL_GetTicks() / 500.0f + (plane_model->vertices[i].x * 2.f)) * 0.25f) /
                                  (0.5f + cosf(SDL_GetTicks() / 200.0f + (plane_model->vertices[i].z * 2.f)) * 0.25f);
  }

  // Update camera matrices
  update_camera(state, camera);
}

// Render the current frame
static usize render_frame(game_state_t* state, transform_t* camera, model_t* cube_model, model_t* plane_model) {
  // Clear framebuffer and reset depth buffer
  for(int i = 0; i < WIN_WIDTH * WIN_HEIGHT; ++i) {
    state->framebuffer[i] = rgb_to_888(FOG_R, FOG_G, FOG_B); // Clear to fog color
    state->depthbuffer[i] = FLT_MAX;
  }
  
  usize rendered = render_model(state, camera, plane_model);
  rendered += render_model(state, camera, cube_model);

  apply_fog_to_screen(state);
  // Update SDL texture and present
  SDL_UpdateTexture(state->framebuffer_tex, NULL, state->framebuffer, WIN_WIDTH * sizeof(u32));
  SDL_RenderTexture(state->renderer, state->framebuffer_tex, NULL, NULL);
  SDL_RenderPresent(state->renderer);

  return rendered;
}

// Cleanup and free SDL resources on exit
static void system_cleanup(game_state_t* state) {
  assert(state != NULL);
  
  if (state->framebuffer_tex) {
    SDL_DestroyTexture(state->framebuffer_tex);
    state->framebuffer_tex = NULL;
  }
  
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
  
  if (system_init(&state) != SDL_APP_SUCCESS) {
    return -1;
  }
  
  // Create framebuffer texture
  state.framebuffer_tex = SDL_CreateTexture(state.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIN_WIDTH, WIN_HEIGHT);
  assert(state.framebuffer_tex != NULL);
  
  // Use nearest-neighbor scaling for pixelated look
  SDL_SetTextureScaleMode(state.framebuffer_tex, SDL_SCALEMODE_NEAREST);
  
  init_renderer(&state);
  
  state.texture_atlas = files[0].data; // Use the first file as texture atlas
  
  // Initialize game objects
  model_t cube_model = {0};
  cube_model.vertices = cube_vertices;
  cube_model.num_vertices = CUBE_VERTEX_COUNT;
  cube_model.scale = make_float3(1.0f, 1.0f, 1.0f);
  cube_model.transform.position = make_float3(0.0f, -1.0f, -5.0f);
  cube_model.uvs = cube_uvs;
  cube_model.num_uvs = 36;
  cube_model.use_textures = true;
  cube_model.frag_shader = &default_shader; // Use default shader

  
  model_t plane_model = {0};
  if (generate_plane(&plane_model, (float2){15.0f, 15.0f}, (float2){1, 1}, (float3){0.0f, 4.0f, 3.0f}) != 0) {
    SDL_Log("Failed to generate plane model");
    system_cleanup(&state);
    return -1;
  }

  shader_t plane_frag_shader = make_shader(plane_frag_shader_func, &plane_model.transform.position, sizeof(float3 *) / sizeof(void *));
  plane_model.use_textures = false;
  plane_model.frag_shader = &plane_frag_shader;
  
  transform_t camera = {0};
  camera.position = make_float3(0.0f, -1.0f, 0.0f);
  camera.yaw = 0.0f;
  camera.pitch = 0.0f;
  
  update_camera(&state, &camera);
  state.running = true;
  
  // Fixed timestep loop variables
  f64 current_time = SDL_GetTicks() / 1000.0;
  f64 accumulator = 0.0;
  
  // Performance tracking
  int frame_count = 0, tick_count = 0;
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
      update_game(&state, &camera, &cube_model, &plane_model, FIXED_TIMESTEP);
      accumulator -= FIXED_TIMESTEP;
      tick_count++;
    }
    
    u64 frame_start = SDL_GetTicks();
    SDL_RenderClear(state.renderer);
    usize tris_rendered = render_frame(&state, &camera, &cube_model, &plane_model);
    u64 frame_end = SDL_GetTicks();
    
    // FPS counter
    frame_count++;
    if (current_time - fps_timer >= 1.0) {
      SDL_Log("FPS: %d, reporting frame took %llu ms", frame_count, frame_end - frame_start);
      SDL_Log("     Reporting frame rendered %u triangles.", tris_rendered);
      SDL_Log("TPS: %d", tick_count);
      frame_count = 0;
      tick_count = 0;
      fps_timer = current_time;
    }
    
    SDL_Delay(1); // Small delay to prevent 100% CPU usage
  }
  
  system_cleanup(&state);
  return 0;
}