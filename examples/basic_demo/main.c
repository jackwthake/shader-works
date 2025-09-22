#include <SDL3/SDL.h>

#include <stdlib.h>
#include <assert.h>
#include <float.h>
#include <math.h>

#include <cpu-render/renderer.h>
#include <cpu-render/maths.h>
#include <cpu-render/primitives.h>

#include "def.h"
#include "resources.inl"


// Simple red shader
static inline u32 frag_cube_func(u32 input, fragment_context_t context, void *argv, usize argc) {
  if (input == 0x00000000)
    return 0x00000000;
  
  u32 output = rgb_to_u32(50, 50, 150);

  output = default_lighting_frag_shader.func(output, context, argv, argc);
  return output;
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
static void update_game(game_state_t* state, transform_t* camera, model_t* cube_model, model_t* plane_model, model_t* sphere_model, model_t* billboard_model, f64 dt) {
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

  // Animate billboard particle - floating motion
  float time = SDL_GetTicks() / 1000.0f;
  billboard_model->transform.position.x = sinf(time * 0.5f) * 2.0f;
  billboard_model->transform.position.y = 3.0f + sinf(time * 1.0f) * 0.5f;
  billboard_model->transform.position.z = -5.0f + cosf(time * 0.3f) * 1.0f;

  // Tggle wireframe mode with space key
  const bool *state_key = SDL_GetKeyboardState(NULL);
  if (state_key[SDL_SCANCODE_SPACE]) {
    state->renderer_state.wireframe_mode = !state->renderer_state.wireframe_mode;
  }

  // Update camera matrices
  update_camera(&state->renderer_state, camera);
}

// Render the current frame
static usize render_frame(game_state_t* state, transform_t* camera, model_t* cube_model, model_t* plane_model, model_t* sphere_model, model_t* billboard_model, light_t* lights, usize light_count) {
  // Clear framebuffer and reset depth buffer
  for(int i = 0; i < WIN_WIDTH * WIN_HEIGHT; ++i) {
    state->framebuffer[i] = rgb_to_u32(FOG_R, FOG_G, FOG_B); // Clear to fog color
    state->depthbuffer[i] = FLT_MAX;
  }
  
  u64 plane_start, plane_end, cube_start, cube_end, sphere_start, sphere_end, billboard_start, billboard_end, fog_start, fog_end;
  u64 sdl_calls_start, sdl_calls_end;

  usize rendered = 0;

  plane_start = SDL_GetTicks();
  rendered += render_model(&state->renderer_state, camera, plane_model, lights, light_count);
  plane_end = SDL_GetTicks();

  cube_start = SDL_GetTicks();
  rendered += render_model(&state->renderer_state, camera, cube_model, lights, light_count);
  cube_end = SDL_GetTicks();

  sphere_start = SDL_GetTicks();
  rendered += render_model(&state->renderer_state, camera, sphere_model, lights, light_count);
  sphere_end = SDL_GetTicks();

  // Render billboard LAST so it appears on top (before fog)
  billboard_start = SDL_GetTicks();
  rendered += render_model(&state->renderer_state, camera, billboard_model, NULL, 0);
  billboard_end = SDL_GetTicks();

  fog_start = SDL_GetTicks();
  apply_fog_to_screen(&state->renderer_state, FOG_START, FOG_END, FOG_R, FOG_G, FOG_B);
  fog_end = SDL_GetTicks();

  // Update SDL texture and present
  sdl_calls_start = SDL_GetTicks();
  SDL_UpdateTexture(state->framebuffer_tex, NULL, state->framebuffer, WIN_WIDTH * sizeof(u32));
  SDL_RenderTexture(state->renderer, state->framebuffer_tex, NULL, NULL);
  SDL_RenderPresent(state->renderer);
  sdl_calls_end = SDL_GetTicks();

  if (SDL_GetTicks() % 100 == 0) {
    SDL_Log("[%llu] Frame Time Summary:", SDL_GetTicks());
    SDL_Log("     Plane model took %llu ms to render", plane_end - plane_start);
    SDL_Log("     Cube model took %llu ms to render", cube_end - cube_start);
    SDL_Log("     Sphere model took %llu ms to render", sphere_end - sphere_start);
    SDL_Log("     Billboard particle took %llu ms to render", billboard_end - billboard_start);
    SDL_Log("     Fog effect took %llu ms to render", fog_end - fog_start);
    SDL_Log("     SDL Renderer/Texture calls took %llu ms to update", sdl_calls_end - sdl_calls_start);
  }

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
  state.framebuffer_tex = SDL_CreateTexture(state.renderer, SDL_PIXELFORMAT_RGBA8888,
                                            SDL_TEXTUREACCESS_STREAMING, WIN_WIDTH, WIN_HEIGHT);
  assert(state.framebuffer_tex != NULL);
  
  // Use nearest-neighbor scaling for pixelated look
  SDL_SetTextureScaleMode(state.framebuffer_tex, SDL_SCALEMODE_NEAREST);

  init_renderer(&state.renderer_state, WIN_WIDTH, WIN_HEIGHT, ATLAS_WIDTH_PX, 
                ATLAS_HEIGHT_PX, state.framebuffer, state.depthbuffer, MAX_DEPTH);
  state.renderer_state.texture_atlas = files[0].data; // Use the first file as texture atlas
  state.renderer_state.wireframe_mode = false; // Start in normal rendering mode

  fragment_shader_t frag_r = make_fragment_shader(frag_cube_func, NULL, 0);
  fragment_shader_t frag_g = make_fragment_shader(frag_plane_func, NULL, 0);
  fragment_shader_t frag_b = make_fragment_shader(frag_sphere_func, NULL, 0);
  fragment_shader_t particle_frag = make_fragment_shader(particle_frag_func, NULL, 0);

  // Create vertex shaders
  vertex_shader_t plane_ripple_vs = make_vertex_shader(plane_ripple_vertex_shader, NULL, 0);
  vertex_shader_t sphere_blob_vs = make_vertex_shader(sphere_blob_vertex_shader, NULL, 0);
  vertex_shader_t billboard_vs = make_vertex_shader(billboard_vertex_shader, NULL, 0);

  // Initialize game objects
  // Generate the cube model
  model_t cube_model = {0};
  if (generate_cube(&cube_model, make_float3(0.0f, 2.0f, -6.0f), make_float3(1.0f, 1.0f, 1.0f)) != 0) {
    SDL_Log("Failed to generate cube model");
    system_cleanup(&state);
    return -1;
  }

  cube_model.frag_shader = &frag_r;
  cube_model.vertex_shader = &default_vertex_shader; // Use default vertex shader
  cube_model.use_textures = true;
  
  model_t plane_model = {0};
  if (generate_plane(&plane_model, (float2){20.0f, 20.0f}, (float2){1, 1}, (float3){0.0f, 0.0f, -10.0f}) != 0) {
    SDL_Log("Failed to generate plane model");
    system_cleanup(&state);
    return -1;
  }

  plane_model.frag_shader = &frag_g;
  plane_model.vertex_shader = &plane_ripple_vs; // Use ripple vertex shader
  plane_model.use_textures = true;

  model_t sphere_model = {0};
  if (generate_sphere(&sphere_model, 1.0f, 32, 32, make_float3(2.0f, 3.0f, -6.0f)) != 0) {
    SDL_Log("Failed to generate sphere model");
    system_cleanup(&state);
    return -1;
  }

  sphere_model.frag_shader = &frag_b;
  sphere_model.vertex_shader = &sphere_blob_vs; // Use blob vertex shader
  sphere_model.use_textures = true;

  // Create billboard particle - normal size now that it's working
  model_t billboard_model = {0};
  if (generate_billboard(&billboard_model, make_float2(1.0f, 1.0f), make_float3(0.0f, 2.0f, -4.0f)) != 0) {
    SDL_Log("Failed to generate billboard model");
    system_cleanup(&state);
    return -1;
  }

  billboard_model.frag_shader = &particle_frag;
  billboard_model.vertex_shader = &billboard_vs; // Use camera-facing vertex shader
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
  camera.yaw = 0.0f;
  camera.pitch = 0.0f;
  
  update_camera(&state.renderer_state, &camera);
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
      update_game(&state, &camera, &cube_model, &plane_model, &sphere_model, &billboard_model, FIXED_TIMESTEP);
      accumulator -= FIXED_TIMESTEP;
      tick_count++;
    }
    
    u64 frame_start = SDL_GetTicks();
    SDL_RenderClear(state.renderer);
    usize tris_rendered = render_frame(&state, &camera, &cube_model, &plane_model, &sphere_model, &billboard_model, lights, num_lights);
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
  delete_model(&cube_model);
  delete_model(&plane_model);
  delete_model(&sphere_model);
  delete_model(&billboard_model);

  system_cleanup(&state);
  return 0;
}