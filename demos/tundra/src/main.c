#include <shader-works/renderer.h>

#include <assert.h>
#include <float.h> // FLT_MAX
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>

#include "util/config.h"
#include "util/state.h"
#include "scene.h"

// Default values
#define MAX_DEPTH 40

typedef enum {
  GENERATE,
  NORMAL,
  OVERHEAD,
  NUM_STATES
} state;

typedef struct {
  uint64_t fps_counter;
  uint64_t tps_counter;
  uint64_t triangle_counter;
  uint64_t last_counter_time;
} performance_counter;

struct context_t {
  u32 *framebuffer;
  f32 *depth_buffer;

  renderer_t renderer;
  scene_t scene;
  const bool *keys;

  float total_time;

  state_machine_t *sm;
};

static const float TICK_RATE = 20.0f; // 20 TPS
static const float TICK_INTERVAL = 1.0f / TICK_RATE;

// Day/night cycle color keyframes: Dawn -> Noon -> Dusk -> Midnight
static const u8 sun_colors[][3] = {
  {30, 50, 120},    // Dawn: deep blue
  {200, 160, 160},  // Noon: white
  {255, 100, 150},  // Dusk: light pink
  {20, 20, 100},    // Midnight: dark blue
};

static const u8 fog_colors[][3] = {
  {20, 30, 80},     // Dawn: rich deep blue
  {240, 245, 250},  // Noon: almost white
  {255, 150, 80},   // Dusk: pastel orange
  {0, 0, 0},        // Midnight: black
};

static void SDL_library_init(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **frame_buf, const char *title, int width, int height, int scale) {
  SDL_Init(SDL_INIT_VIDEO);

  SDL_CreateWindowAndRenderer(title, width * scale, height * scale, 0, window, renderer);

  *frame_buf = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
  SDL_SetTextureScaleMode(*frame_buf, SDL_SCALEMODE_NEAREST);

  SDL_SetWindowRelativeMouseMode(*window, false);
}

static void init_performance_counter(performance_counter *stats) {
  stats->fps_counter = 0;
  stats->tps_counter = 0;
  stats->triangle_counter = 0;
  stats->last_counter_time = SDL_GetPerformanceCounter();
}

static void get_cycle_color(float time_elapsed, const u8 colors[][3], u8 *r, u8 *g, u8 *b) {
  const float CYCLE_DURATION = 120.0f; // 2 minutes total
  const int NUM_PHASES = 4;

  float cycle_time = fmodf(time_elapsed, CYCLE_DURATION);
  float phase = (cycle_time / CYCLE_DURATION) * NUM_PHASES;

  int idx = (int)phase;
  int next_idx = (idx + 1) % NUM_PHASES;
  float t = phase - idx;

  *r = (u8)lerp(colors[idx][0], colors[next_idx][0], t);
  *g = (u8)lerp(colors[idx][1], colors[next_idx][1], t);
  *b = (u8)lerp(colors[idx][2], colors[next_idx][2], t);
}

static u32 get_sun_color(float time_elapsed) {
  u8 r, g, b;
  get_cycle_color(time_elapsed, sun_colors, &r, &g, &b);
  return rgb_to_u32(r, g, b);
}

static void get_fog_color(float time_elapsed, u8 *r, u8 *g, u8 *b) {
  get_cycle_color(time_elapsed, fog_colors, r, g, b);
}

static void apply_fps_movement(struct context_t *ctx, float dt) {
  const bool *keys = ctx->keys;
  float3 movement = {0}, right, up, forward;
  float speed = ctx->scene.controller.move_speed * dt;
  
  // movement
  transform_get_basis_vectors(&ctx->scene.camera_pos, &right, &up, &forward);

  if (keys[SDL_SCANCODE_W]) movement = float3_add(movement, float3_scale(forward, -speed));
  if (keys[SDL_SCANCODE_S]) movement = float3_add(movement, float3_scale(forward, speed));
  if (keys[SDL_SCANCODE_A]) movement = float3_add(movement, float3_scale(right, speed));
  if (keys[SDL_SCANCODE_D]) movement = float3_add(movement, float3_scale(right, -speed));
  
  // apply movement
  ctx->scene.camera_pos.position = float3_add(ctx->scene.camera_pos.position, movement);
  float new_ground_height = get_interpolated_terrain_height(ctx->scene.camera_pos.position.x, ctx->scene.camera_pos.position.z);

  // mouse input
  float mx, my;
  SDL_GetRelativeMouseState(&mx, &my);
  
  ctx->scene.camera_pos.yaw += mx * ctx->scene.controller.mouse_sensitivity;
  ctx->scene.camera_pos.pitch -= my * ctx->scene.controller.mouse_sensitivity;
  
  if (ctx->scene.camera_pos.pitch < ctx->scene.controller.min_pitch) ctx->scene.camera_pos.pitch = ctx->scene.controller.min_pitch;
  if (ctx->scene.camera_pos.pitch > ctx->scene.controller.max_pitch) ctx->scene.camera_pos.pitch = ctx->scene.controller.max_pitch;

  ctx->scene.controller.ground_height = new_ground_height;
  update_camera(&ctx->renderer, &ctx->scene.camera_pos);
}

static void on_generate(void *args, size_t size) {
  (void)size; // unused
  struct context_t *ctx = (struct context_t *)args;

  if (!ctx) return;

  ctx->scene = (scene_t) {
    .camera_pos = { 0 },
    .chunk_map = { 0 },
    .controller = (fps_controller_t) {
      .move_speed = 15.0f,
      .mouse_sensitivity = 0.002f,
      .min_pitch = -PI / 2 + EPSILON,
      .max_pitch = PI / 2  - EPSILON,
      .camera_height_offset = 3.0f,
      .delta_time = TICK_INTERVAL,
      .last_frame_time = 0.0f,
      .ground_height = 0.0f
    }
  };

  init_scene(&ctx->scene, g_world_config.max_chunks);

  // Set initial camera position and height
  float terrain_height = get_interpolated_terrain_height(0.0f, 0.0f);
  ctx->scene.controller.ground_height = terrain_height;
  ctx->scene.camera_pos.position.y = terrain_height + ctx->scene.controller.camera_height_offset;

  fsm_change_state(ctx->sm, NORMAL);
}

static void on_normal_enter(void *args, size_t size) {
  (void)size; // unused
  if (!args) return;

  struct context_t *ctx = (struct context_t*)args;

  ctx->renderer.max_depth = MAX_DEPTH;
  ctx->scene.sun = (light_t) {
    .is_directional = true,
    .direction = make_float3(1, -1, 1),
    .color = rgb_to_u32(200, 160, 160)
  };

  ctx->renderer.wireframe_mode = false;

  // Update camera with current position
  update_camera(&ctx->renderer, &ctx->scene.camera_pos);
}

static void on_normal_tick(void *args, size_t size, float dt) {
  (void)size; // unused
  if (!args) return;

  struct context_t *ctx = (struct context_t*)args;
  
  // apply movement
  apply_fps_movement(ctx, dt);
  ctx->scene.camera_pos.position.y = ctx->scene.controller.ground_height + ctx->scene.controller.camera_height_offset;

  // update snow particles
  update_quads(ctx->scene.camera_pos.position, &ctx->scene.camera_pos);

  // update loaded chunks
  update_loaded_chunks(&ctx->scene);

  ctx->scene.sun.color = get_sun_color(ctx->total_time);
}

static int on_normal_render(void *args, size_t size) {
  (void)size; // unused
  if (!args) return 0;

  struct context_t *ctx = (struct context_t*)args;

  usize triangles_rendered = render_loaded_chunks(&ctx->renderer, &ctx->scene, &ctx->scene.sun, 1);
  triangles_rendered += render_quads(&ctx->renderer, &ctx->scene.camera_pos, &ctx->scene.sun, 1);

  u8 fog_r, fog_g, fog_b;
  get_fog_color(ctx->total_time, &fog_r, &fog_g, &fog_b);
  apply_fog_to_screen(&ctx->renderer, ctx->renderer.max_depth / 2.f, ctx->renderer.max_depth - 1.0f, fog_r, fog_g, fog_b);

  return triangles_rendered;
}

static void on_overhead_enter(void *args, size_t size) {
  (void)size; // unused
  if (!args) return;

  struct context_t *ctx = (struct context_t*)args;

  ctx->scene.camera_pos.position.y += 45.0f;
  ctx->scene.camera_pos.pitch = -PI / 2;
  ctx->scene.camera_pos.yaw = 0;

  ctx->renderer.max_depth = 250;
  ctx->renderer.wireframe_mode = false;
}

static void on_overhead_tick(void *args, size_t size, float dt) {
  (void)size; // unused
  if (!args) return;

  struct context_t *ctx = (struct context_t*)args;

  float3 world_forward = make_float3(0, 0, -1); // Forward is negative Z (up on screen)
  float3 world_right = make_float3(1, 0, 0);    // Right is positive X
  float3 movement = {0};
  float speed = ctx->scene.controller.move_speed * dt;
  const bool *keys = ctx->keys;


  if (keys[SDL_SCANCODE_W]) movement = float3_add(movement, float3_scale(world_forward, speed));
  if (keys[SDL_SCANCODE_S]) movement = float3_add(movement, float3_scale(world_forward, -speed));
  if (keys[SDL_SCANCODE_A]) movement = float3_add(movement, float3_scale(world_right, speed));
  if (keys[SDL_SCANCODE_D]) movement = float3_add(movement, float3_scale(world_right, -speed));

  ctx->scene.camera_pos.position = float3_add(ctx->scene.camera_pos.position, movement);
  update_loaded_chunks(&ctx->scene);
  update_camera(&ctx->renderer, &ctx->scene.camera_pos);
}

static int on_overhead_render(void *args, size_t size) {
  (void)size; // unused
  if (!args) return 0;

  struct context_t *ctx = (struct context_t*)args;
  
  model_t cube = { 0 };
  float3 pos = make_float3(ctx->scene.camera_pos.position.x, ctx->scene.controller.ground_height + ctx->scene.controller.camera_height_offset, ctx->scene.camera_pos.position.z);
  generate_cube(&cube, pos, (float3){ 2, 1, 2 });

  usize triangles_rendered = render_loaded_chunks(&ctx->renderer, &ctx->scene, &ctx->scene.sun, 1);
  return triangles_rendered + render_model(&ctx->renderer, &ctx->scene.camera_pos, &cube, &ctx->scene.sun, 1);
}

static state_interface_t generate = {
  .enter = on_generate,
  .tick = NULL,
  .render = NULL,
  .exit = NULL,
};

static state_interface_t normal = {
  .enter = on_normal_enter,
  .tick = on_normal_tick,
  .render = on_normal_render,
  .exit = NULL
};

static state_interface_t overhead = {
  .enter = on_overhead_enter,
  .tick = on_overhead_tick,
  .render = on_overhead_render,
  .exit = NULL
};

int main(int argc, char const *argv[]) {
  (void)argc; (void)argv;

  // Load window configuration from config.json
  unsigned int config_width, config_height, config_scale;
  char config_title[256];

  load_config(&config_width, &config_height, &config_scale, config_title, sizeof(config_title));
  load_world_config();

  SDL_Window *sdl_window;
  SDL_Renderer *sdl_renderer;
  SDL_Texture *sdl_framebuff;

  u32 *framebuffer = (u32 *)malloc(config_width * config_height * sizeof(u32));
  f32 *depth_buffer = (f32 *)malloc(config_width * config_height * sizeof(f32));

  // Initialize state and window
  SDL_library_init(&sdl_window, &sdl_renderer, &sdl_framebuff, config_title, config_width, config_height, config_scale);
  SDL_SetWindowRelativeMouseMode(sdl_window, true);

  renderer_t renderer = {0};
  init_renderer(&renderer, config_width, config_height, 0, 0, framebuffer, depth_buffer, MAX_DEPTH);

  performance_counter stats;
  init_performance_counter(&stats);

  // Initialize state machine
  state_machine_t sm = {0};
  fsm_init(&sm, GENERATE, NUM_STATES);

  struct context_t state_context = {
    .framebuffer = framebuffer,
    .depth_buffer = depth_buffer,
    .renderer = renderer,

    .keys = SDL_GetKeyboardState(NULL),
    .sm = &sm,
    .total_time = 0.0f
  };

  fsm_set_state_interface(&sm, GENERATE, &generate);
  fsm_set_state_interface(&sm, NORMAL, &normal);
  fsm_set_state_interface(&sm, OVERHEAD, &overhead);

  fsm_update_internal_state(&sm, &state_context, sizeof(struct context_t));
  fsm_start(&sm);

  bool running = true;
  float accumulator = 0.0f;
  uint64_t last_time = SDL_GetPerformanceCounter();

  while (running) {
    uint64_t current_time = SDL_GetPerformanceCounter();
    float frame_time = (float)(current_time - last_time) / (float)SDL_GetPerformanceFrequency();
    last_time = current_time;

    // Cap frame time to prevent spiral of death
    if (frame_time > 0.1f) frame_time = 0.1f;

    accumulator += frame_time;
    state_context.total_time += frame_time;
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) running = false;
      if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.key == SDLK_ESCAPE) {
          running = false;
        } 
        
        if (event.key.key == SDLK_1 && fsm_get_state(&sm) != NORMAL)
          fsm_change_state(&sm, NORMAL);
        else if (event.key.key == SDLK_2)
          fsm_change_state(&sm, OVERHEAD);
        else if (event.key.key == SDLK_3)
          renderer.wireframe_mode = !renderer.wireframe_mode;
      }
    }

    // Fixed timestep game updates
    while (accumulator >= TICK_INTERVAL) {
      fsm_tick_state(&sm, TICK_INTERVAL);

      accumulator -= TICK_INTERVAL;
      stats.tps_counter++;
    }

    u8 bg_r, bg_g, bg_b;
    get_fog_color(state_context.total_time, &bg_r, &bg_g, &bg_b);
    u32 background_color = rgb_to_u32(bg_r, bg_g, bg_b);

    for(int i = 0; i < config_width * config_height; ++i) {
      framebuffer[i] = background_color;
      depth_buffer[i] = FLT_MAX;
    }

    fsm_render_state(&sm);

    SDL_UpdateTexture(sdl_framebuff, NULL, framebuffer, config_width * sizeof(u32));
    SDL_RenderTexture(sdl_renderer, sdl_framebuff, NULL, NULL);
    SDL_RenderPresent(sdl_renderer);
    
    stats.fps_counter++;
    // stats.triangle_counter += triangles_rendered;
    uint64_t counter_time = SDL_GetPerformanceCounter();
    if ((float)(counter_time - stats.last_counter_time) / (float)SDL_GetPerformanceFrequency() >= 1.0f) {
      uint64_t avg_triangles_per_frame = stats.fps_counter > 0 ? stats.triangle_counter / stats.fps_counter : 0;
      printf("TPS: %lu, FPS: %lu, Triangles/frame: %lu, Player: (%.1f, %.1f, %.1f)\n",
              stats.tps_counter, stats.fps_counter, avg_triangles_per_frame,
              state_context.scene.camera_pos.position.x, state_context.scene.camera_pos.position.y, state_context.scene.camera_pos.position.z);
      stats.tps_counter = 0;
      stats.fps_counter = 0;
      stats.triangle_counter = 0;
      stats.last_counter_time = counter_time;
    }
  }

  free_chunk_map(&state_context.scene.chunk_map);
  fsm_free(&sm);

  free(framebuffer);
  free(depth_buffer);

  SDL_DestroyTexture(sdl_framebuff);
  SDL_DestroyRenderer(sdl_renderer);
  SDL_DestroyWindow(sdl_window);
  SDL_Quit();

  free_config();

  return 0;
}
