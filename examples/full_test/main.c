#include <SDL3/SDL.h>

#include <stdio.h>
#include <assert.h>
#include <float.h>
#include <math.h>

#include <cpu-render/renderer.h>
#include <cpu-render/maths.h>
#include <cpu-render/primitives.h>

#include "def.h"
#include "resources.inl"


// Simple blue shader
static inline u32 frag_cube_func(u32 input, fragment_context_t context, void *argv, usize argc) {
  if (input == 0x00000000) return 0x00000000;
  return default_lighting_frag_shader.func(rgb_to_u32(50, 50, 150), context, argv, argc);
}

// soft green shader with noise based on depth
static inline u32 frag_plane_func(u32 input, fragment_context_t context, void *argv, usize argc) {
  if (input == 0x00000000)
    return 0x00000000;

  // Pixelated, fast-scrolling noise using quantized world position and time
  float pixel_size = 0.1f; // Controls pixelation
  float quant_x = floorf(context.world_pos.x / pixel_size) * pixel_size;
  float quant_y = floorf(context.world_pos.y / pixel_size) * pixel_size;
  float quant_z = floorf(context.world_pos.z / pixel_size) * pixel_size;

  float noise_x = sinf(quant_x * 8.0f + context.time * 15.0f) *
                  cosf(quant_z * 6.0f + context.time * 14.0f);
  float noise_y = sinf(quant_y * 10.0f + context.time * 16.0f) *
                  cosf(quant_x * 7.0f);

  float noise = (noise_x + noise_y) * 0.5f;

  u8 r = (u8)fmaxf(0.0f, fminf(170.0f, 50.0f + noise * 100.0f));
  u8 g = (u8)fmaxf(0.0f, fminf(255.0f, 200.0f + noise * 80.0f));
  u8 b = (u8)fmaxf(0.0f, fminf(150.0f, 30.0f + noise * 60.0f));

  u32 output = default_lighting_frag_shader.func(rgb_to_u32(r, g, b), context, argv, argc);
  return output;
}

// soft blue shader with depth and time-based animation
static inline u32 frag_sphere_func(u32 input, fragment_context_t context, void *argv, usize argc) {
  if (input == 0x00000000)
    return 0x00000000;

  // Demonstrate context usage - animate color based on depth and time
  float depth_factor = fmaxf(0.0f, fminf(1.0f, context.world_pos.y / 2.0f));
  float time_wave = (sinf(context.time * 2.0f) + 1.0f) * 0.5f;

  u8 r = (u8)(100 + depth_factor * 155 * time_wave);
  u8 g = (u8)(100 + (1.0f - depth_factor) * 155);
  u8 b = (u8)(255 - depth_factor * 100);

  u32 output = default_lighting_frag_shader.func(rgb_to_u32(r, g, b), context, argv, argc);
  return output;
}

// Vertex shader for plane ripple effect
static inline float3 plane_ripple_vertex_shader(vertex_context_t context, void *argv, usize argc) {
  float3 vertex = context.original_vertex;

  // Apply the same ripple effect that was previously done in CPU
  vertex.y = (sinf(context.time + vertex.x) * 0.25f) /
             (sinf(context.time * 5.0f + (vertex.x * 2.0f)) / 4.0f + 1.0f);

  // Update normal to reflect the rippled surface
  float delta = 0.01f;
  float3 p_dx = (float3){ vertex.x + delta, 0, vertex.z };
  float3 p_dz = (float3){ vertex.x, 0, vertex.z + delta };
  p_dx.y = (sinf(context.time + p_dx.x) * 0.25f) /
           (sinf(context.time * 5.0f + (p_dx.x * 2.0f)) / 4.0f + 1.0f);
  p_dz.y = (sinf(context.time + p_dz.x) * 0.25f) /
           (sinf(context.time * 5.0f + (p_dz.x * 2.0f)) / 4.0f + 1.0f);
  float3 tangent = float3_sub(p_dx, vertex);
  float3 bitangent = float3_sub(p_dz, vertex);
  float3 normal = float3_cross(tangent, bitangent);
  normal = float3_normalize(normal);

  *context.original_normal = normal;

  return vertex;
}

// Vertex shader for sphere blob effect
static inline float3 sphere_blob_vertex_shader(vertex_context_t context, void *argv, usize argc) {
  float3 vertex = context.original_vertex;

  // Calculate distance from origin for noise basis
  float base_distance = sqrtf(vertex.x * vertex.x + vertex.y * vertex.y + vertex.z * vertex.z);

  // Multiple noise frequencies for organic blob effect
  float noise1 = sinf(context.time * 2.0f + vertex.x * 4.0f + vertex.y * 3.0f + vertex.z * 2.0f);
  float noise2 = sinf(context.time * 1.5f + vertex.y * 5.0f + vertex.z * 4.0f);
  float noise3 = cosf(context.time * 3.0f + vertex.z * 3.0f + vertex.x * 2.0f);

  // Combine noise for organic deformation
  float combined_noise = (noise1 * 0.4f + noise2 * 0.3f + noise3 * 0.3f);

  // Add slow breathing motion
  float breathing = sinf(context.time * 0.8f) * 0.015f;

  // Calculate deformation factor - stronger at poles, weaker at equator
  float pole_factor = fabsf(vertex.y) / base_distance;
  float deform_strength = 0.3f + pole_factor * 0.2f;

  // Apply deformation along the normal direction (original vertex is normalized for sphere)
  float3 normal = vertex;  // For unit sphere, position is the normal
  if (base_distance > 0.001f) {
    normal.x /= base_distance;
    normal.y /= base_distance;
    normal.z /= base_distance;
  }

  // Apply the blob deformation with bounds checking
  float displacement = (combined_noise * deform_strength + breathing) * 0.25f;

  // Clamp displacement to prevent extreme deformations
  displacement = fmaxf(-0.4f, fminf(0.4f, displacement));

  vertex.x += normal.x * displacement;
  vertex.y += normal.y * displacement;
  vertex.z += normal.z * displacement;

  return vertex;
}

// Billboard vertex shader - makes quads always face the camera
static inline float3 billboard_vertex_shader(vertex_context_t context, void *argv, usize argc) {
  float3 vertex = context.original_vertex;

  // Get camera vectors from context
  float3 cam_right = context.cam_right;
  float3 cam_up = context.cam_up;

  // Billboard position is stored in the model transform
  // The vertex coordinates represent offsets from the billboard center
  float3 world_pos = float3_add(
    float3_add(
      float3_scale(cam_right, vertex.x),  // X offset using camera right vector
      float3_scale(cam_up, vertex.y)     // Y offset using camera up vector
    ),
    make_float3(0.0f, 0.0f, 0.0f)        // Billboard center will be added by transform
  );

  return world_pos;
}

// Particle fragment shader - circular particle
static inline u32 particle_frag_func(u32 input, fragment_context_t context, void *argv, usize argc) {
  // Center the UV coordinates around (0.5, 0.5)
  f32 dx = context.uv.x - 0.5f;
  f32 dy = context.uv.y - 0.5f;
  f32 dist = sqrtf(dx * dx + dy * dy);

  // Use maximum radius to fill most of the billboard
  f32 radius = 0.5f;

  // Hard circle cutoff - discard pixels outside circle
  if (dist > radius)
    return rgb_to_u32(255, 0, 255); // Transparent (discard pixel)

  return input; // Use input texture color
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
  rendered += render_model(&state->renderer_state, camera, plane_model, lights, light_count);
  rendered += render_model(&state->renderer_state, camera, cube_model, lights, light_count);
  rendered += render_model(&state->renderer_state, camera, sphere_model, lights, light_count);
  rendered += render_model(&state->renderer_state, camera, billboard_model, NULL, 0);

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
  generate_billboard(&billboard_model, make_float2(1.0f, 1.0f), make_float3(0.0f, 2.0f, -4.0f));
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