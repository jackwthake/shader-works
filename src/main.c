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
static void update_game(game_state_t* state, transform_t* camera, model_t* cube_model, model_t* plane_model, model_t* sphere_model, f64 dt) {
  // dt will always be FIXED_TIMESTEP
  
  // rotate cube
  cube_model->transform.yaw  += 1.0f * dt;
  cube_model->transform.pitch += 0.5f * dt;
  
  // slowly move cube back and forth
  cube_model->transform.position.z = ((sinf(SDL_GetTicks() / 1000.0f) * 4.f) - 7.0f);

  // slowly bob sphere and rotate
  sphere_model->transform.position.y = (sinf(SDL_GetTicks() / 1000.0f) * 1.f) + 3.0f;
  sphere_model->transform.yaw  += 0.5f * dt;
  sphere_model->transform.pitch += 0.5f * dt;

  // wripple plane vertices based off of time
  for (int i = 0; i < plane_model->num_vertices; ++i) {
    plane_model->vertices[i].y = (sinf(SDL_GetTicks() / 1000.0f + (plane_model->vertices[i].x)) * 0.25f) / 
                                 (sinf(SDL_GetTicks() / 200.0f + (plane_model->vertices[i].x * 2.f)) / 4.0f + 1.0f);
  }

  // Update camera matrices
  update_camera(state, camera);
}

// Render the current frame
static usize render_frame(game_state_t* state, transform_t* camera, model_t* cube_model, model_t* plane_model, model_t* sphere_model) {
  // Clear framebuffer and reset depth buffer
  for(int i = 0; i < WIN_WIDTH * WIN_HEIGHT; ++i) {
    state->framebuffer[i] = rgb_to_888(FOG_R, FOG_G, FOG_B); // Clear to fog color
    state->depthbuffer[i] = FLT_MAX;
  }
  
  usize rendered = 0;
  rendered += render_model(state, camera, plane_model); 
  rendered += render_model(state, camera, cube_model);
  rendered += render_model(state, camera, sphere_model);
  
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
  // Generate the cube model
  model_t cube_model = {0};
  if (generate_cube(&cube_model, make_float3(0.0f, 2.0f, -6.0f), make_float3(1.0f, 1.0f, 1.0f)) != 0) {
    SDL_Log("Failed to generate cube model");
    system_cleanup(&state);
    return -1;
  }

  cube_model.frag_shader = &default_shader; // Use default shader
  cube_model.use_textures = true;
  
  model_t plane_model = {0};
  if (generate_plane(&plane_model, (float2){20.0f, 20.0f}, (float2){1, 1}, (float3){0.0f, 0.0f, -10.0f}) != 0) {
    SDL_Log("Failed to generate plane model");
    system_cleanup(&state);
    return -1;
  }

  plane_model.frag_shader = &default_shader;
  plane_model.use_textures = true;

  model_t sphere_model = {0};
  if (generate_sphere(&sphere_model, 1.0f, 16, 16, make_float3(2.0f, 3.0f, -6.0f)) != 0) {
    SDL_Log("Failed to generate sphere model");
    system_cleanup(&state);
    return -1;
  }

  sphere_model.frag_shader = &default_shader;
  sphere_model.use_textures = true;
  
  transform_t camera = {0};
  camera.position = make_float3(0.0f, 2.0f, 0.0f);  // Place camera at origin
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
      update_game(&state, &camera, &cube_model, &plane_model, &sphere_model, FIXED_TIMESTEP);
      accumulator -= FIXED_TIMESTEP;
      tick_count++;
    }
    
    u64 frame_start = SDL_GetTicks();
    SDL_RenderClear(state.renderer);
    usize tris_rendered = render_frame(&state, &camera, &cube_model, &plane_model, &sphere_model);
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
  
  // Cleanup model resources
  free(cube_model.face_normals);
  free(plane_model.vertices);
  free(plane_model.uvs);
  free(plane_model.face_normals);
  
  system_cleanup(&state);
  return 0;
}