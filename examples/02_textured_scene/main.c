#include <SDL3/SDL.h>

#include <stdio.h>
#include <assert.h>
#include <float.h>
#include <math.h>

#include <shader-works/renderer.h>
#include <shader-works/maths.h>
#include <shader-works/primitives.h>

#include "def.h"
#include "resources.inl"
#include "shaders.h"

// Color conversion functions using SDL (required by shader-works)
u32 rgb_to_u32(u8 r, u8 g, u8 b) {
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);
  return SDL_MapRGBA(format, NULL, r, g, b, 255);
}

void u32_to_rgb(u32 color, u8 *r, u8 *g, u8 *b) {
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);
  SDL_GetRGB(color, format, NULL, r, g, b);
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

    if (event.type == SDL_EVENT_KEY_DOWN) {
      if (event.key.key == SDLK_ESCAPE) {
        state->running = false;
      }

      if (event.key.key == SDLK_W) {
        state->renderer_state.wireframe_mode = !state->renderer_state.wireframe_mode;
        printf("Wireframe mode: %s\n", state->renderer_state.wireframe_mode ? "ON" : "OFF");
      }
    }
  }
}

// Update performance counters
static void update_performance_counters(game_state_t* state, f64 current_time) {
  // Update FPS counter every second
  if (current_time - state->last_fps_time >= 1.0) {
    state->current_fps = state->frame_count / (current_time - state->last_fps_time);
    state->frame_count = 0;
    state->last_fps_time = current_time;
  }

  // Update TPS counter every second
  if (current_time - state->last_tps_time >= 1.0) {
    state->current_tps = state->tick_count / (current_time - state->last_tps_time);
    state->tick_count = 0;
    state->last_tps_time = current_time;

    // Print counters to console
    printf("FPS: %.1f | TPS: %.1f\n", state->current_fps, state->current_tps);
  }
}

// Update game logic with fixed timestep
static void update_game(game_state_t* state, transform_t* camera, model_t* cube_model, model_t* plane_model, model_t* sphere_model, model_t* billboard_model, f64 dt) {
  // dt will always be FIXED_TIMESTEP

  // Increment tick counter
  state->tick_count++;

  // rotate cube
  cube_model->transform.yaw  += 1.0f * dt;
  cube_model->transform.pitch += 0.5f * dt;

  // slowly move cube back and forth
  cube_model->transform.position.z = ((sinf(SDL_GetTicks() / 1000.0f) * 4.f) - 7.0f);

  // slowly bob sphere and rotate
  sphere_model->transform.position.y = (sinf(SDL_GetTicks() / 1000.0f) * 1.f) + 3.0f;
  sphere_model->transform.yaw  += 0.5f * dt;
  sphere_model->transform.pitch += 0.5f * dt;

  // Animate billboard particle - floating motion
  float time = SDL_GetTicks() / 1000.0f;
  billboard_model->transform.position.x = sinf(time * 0.5f) * 2.0f;
  billboard_model->transform.position.y = 3.0f + sinf(time * 1.0f) * 0.5f;
  billboard_model->transform.position.z = -5.0f + cosf(time * 0.3f) * 1.0f;


  // Update camera matrices
  update_camera(&state->renderer_state, camera);
}

// Render the current frame
static usize render_frame(game_state_t* state, transform_t* camera, model_t* cube_model, model_t* plane_model, model_t* sphere_model, model_t* billboard_model, light_t* lights, usize light_count) {
  // Clear framebuffer and reset depth buffer
  for(int i = 0; i < WIN_WIDTH * WIN_HEIGHT; ++i) {
    state->framebuffer[i] = rgb_to_u32(FOG_R, FOG_G, FOG_B);
    state->depthbuffer[i] = FLT_MAX;
  }

  usize rendered = 0;
  rendered += render_model(&state->renderer_state, camera, billboard_model, NULL, 0);
  rendered += render_model(&state->renderer_state, camera, cube_model, lights, light_count);
  rendered += render_model(&state->renderer_state, camera, sphere_model, lights, light_count);
  rendered += render_model(&state->renderer_state, camera, plane_model, lights, light_count);

  apply_fog_to_screen(&state->renderer_state, FOG_START, FOG_END, FOG_R, FOG_G, FOG_B);

  // Present framebuffer to screen
  SDL_UpdateTexture(state->framebuffer_tex, NULL, state->framebuffer, WIN_WIDTH * sizeof(u32));
  SDL_RenderTexture(state->renderer, state->framebuffer_tex, NULL, NULL);
  SDL_RenderPresent(state->renderer);

  // Increment frame counter
  state->frame_count++;

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

  // Initialize SDL and create window/renderer
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(WIN_TITLE, WIN_WIDTH * WIN_SCALE, WIN_HEIGHT * WIN_SCALE, 0, &state.window, &state.renderer);

  // Create framebuffer texture with nearest-neighbor scaling
  state.framebuffer_tex = SDL_CreateTexture(state.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIN_WIDTH, WIN_HEIGHT);
  SDL_SetTextureScaleMode(state.framebuffer_tex, SDL_SCALEMODE_NEAREST);

  init_renderer(&state.renderer_state, WIN_WIDTH, WIN_HEIGHT, ATLAS_WIDTH_PX,
                ATLAS_HEIGHT_PX, state.framebuffer, state.depthbuffer, MAX_DEPTH);
  state.renderer_state.texture_atlas = files[0].data;

  fragment_shader_t frag_r = make_fragment_shader(frag_cube_func, NULL, 0);
  fragment_shader_t frag_g = make_fragment_shader(frag_plane_func, NULL, 0);
  fragment_shader_t frag_b = make_fragment_shader(frag_sphere_func, NULL, 0);
  fragment_shader_t particle_frag = make_fragment_shader(particle_frag_func, NULL, 0);

  // Create vertex shaders
  vertex_shader_t plane_ripple_vs = make_vertex_shader(plane_ripple_vertex_shader, NULL, 0);
  vertex_shader_t sphere_blob_vs = make_vertex_shader(sphere_blob_vertex_shader, NULL, 0);
  vertex_shader_t billboard_vs = make_vertex_shader(billboard_vertex_shader, NULL, 0);

  // Initialize game objects
  // Generate models
  model_t cube_model = {0};
  generate_cube(&cube_model, make_float3(0.0f, 2.0f, -6.0f), make_float3(1.0f, 1.0f, 1.0f));
  cube_model.frag_shader = &frag_r;
  cube_model.vertex_shader = &default_vertex_shader;
  cube_model.use_textures = true;

  model_t plane_model = {0};
  generate_plane(&plane_model, (float2){20.0f, 20.0f}, (float2){1, 1}, (float3){0.0f, 0.0f, -10.0f});
  plane_model.frag_shader = &frag_g;
  plane_model.vertex_shader = &plane_ripple_vs;
  plane_model.use_textures = true;

  model_t sphere_model = {0};
  generate_sphere(&sphere_model, 1.0f, 32, 32, make_float3(2.0f, 3.0f, -6.0f));
  sphere_model.frag_shader = &frag_b;
  sphere_model.vertex_shader = &sphere_blob_vs;
  sphere_model.use_textures = true;

  model_t billboard_model = {0};
  generate_quad(&billboard_model, make_float2(1.0f, 1.0f), make_float3(0.0f, 2.0f, -4.0f));
  billboard_model.frag_shader = &particle_frag;
  billboard_model.vertex_shader = &billboard_vs;
  billboard_model.use_textures = true;

  #define num_lights 2
  light_t lights[num_lights] = {
    { // directional light
      .color = rgb_to_u32(255, 255, 255),
      .direction = make_float3(-1.0f, -1.0f, -1.0f),
      .is_directional = true
    },
    {
      .position = make_float3(-1.0f, 2.0f, -5.0f),
      .color = rgb_to_u32(255, 0, 0),
      .is_directional = false
    }
  };

  transform_t camera = {0};
  camera.position = make_float3(0.0f, 2.0f, 0.0f);  // Place camera at origin

  update_camera(&state.renderer_state, &camera);
  state.running = true;

  // Initialize performance counters
  f64 init_time = SDL_GetTicks() / 1000.0;
  state.frame_count = 0;
  state.tick_count = 0;
  state.last_fps_time = init_time;
  state.last_tps_time = init_time;
  state.current_fps = 0.0;
  state.current_tps = 0.0;

  // Fixed timestep loop variables
  f64 current_time = init_time;
  f64 accumulator = 0.0;

  while (state.running) {
    f64 new_time = SDL_GetTicks() / 1000.0;
    f64 frame_time = new_time - current_time;
    current_time = new_time;

    // Prevent spiral of death - cap frame time
    frame_time = fmin(frame_time, MAX_FRAME_TIME);
    accumulator += frame_time;

    // Process input (this can happen at variable rate)
    system_poll_events(&state);

    // Update performance counters
    update_performance_counters(&state, current_time);

    // Fixed timestep updates
    while (accumulator >= FIXED_TIMESTEP) {
      update_game(&state, &camera, &cube_model, &plane_model, &sphere_model, &billboard_model, FIXED_TIMESTEP);
      accumulator -= FIXED_TIMESTEP;
    }

    SDL_RenderClear(state.renderer);
    render_frame(&state, &camera, &cube_model, &plane_model, &sphere_model, &billboard_model, lights, num_lights);

    SDL_Delay(1); // Small delay to prevent 100% CPU usage
  }

  // Cleanup model resources
  delete_model(&cube_model);
  delete_model(&plane_model);
  delete_model(&sphere_model);
  delete_model(&billboard_model);

  system_cleanup(&state);
  return 0;
}
