#include <shader-works/renderer.h>

#include <assert.h>
#include <float.h> // FLT_MAX
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>

#include "common/config.h"
#include "common/noise.h"
#include "util/state.h"
#include "scene.h"

// Default values
#define MAX_DEPTH (g_world_config.chunk_size * g_world_config.chunk_load_radius) - 1

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

static u32 get_sun_color(float time_elapsed) {
  (void)time_elapsed;
  return rgb_to_u32(185.f, 195.f, 235.f);
}

static void get_fog_color(float time_elapsed, u8 *r, u8 *g, u8 *b) {
  (void)time_elapsed;
  *r = 162; *g = 208.f; *b = 227.f;
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
  ctx->scene.controller.distance_walked = sqrtf((movement.x * movement.x) + (movement.z * movement.z));
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

static float3 get_slope_normal(float x, float z, int seed) {
  float hL = terrainHeight(x - EPSILON, z, seed);
  float hR = terrainHeight(x + EPSILON, z, seed);
  float hD = terrainHeight(x, z - EPSILON, seed);
  float hU = terrainHeight(x, z + EPSILON, seed);

  float dz_dx = (hR - hL) / (2.0f * EPSILON);
  float dz_dz = (hU - hD) / (2.0f * EPSILON);

  float3 normal = {-dz_dx, 1.0f, -dz_dz};

  return float3_normalize(normal);
}

static void apply_ski_movement(struct context_t *ctx, float dt) {
  float3 right, up, forward;
  transform_get_basis_vectors(&ctx->scene.camera_pos, &right, &up, &forward);

  float3 normal = get_slope_normal(ctx->scene.camera_pos.position.x, ctx->scene.camera_pos.position.z, g_world_config.seed);
  float3 gravity = { 0.0f, -12.f, 0.0f };
  float slope_dot = float3_dot(gravity, normal);

  float3 downhill_acc = float3_sub(gravity, float3_scale(normal, slope_dot));
  float3 kick_force = {0};

  const bool *keys = ctx->keys;
  if (keys[SDL_SCANCODE_W]) {
    kick_force = float3_scale(forward, -15.f);
  }

  // apply forces
  ctx->scene.controller.velocity = float3_add(ctx->scene.controller.velocity, float3_scale(downhill_acc, dt));
  ctx->scene.controller.velocity = float3_add(ctx->scene.controller.velocity, float3_scale(kick_force, dt));

  // friction
  if (keys[SDL_SCANCODE_S]) {
    ctx->scene.controller.velocity = float3_scale(ctx->scene.controller.velocity, 0.70f);

    if (float3_magnitude(ctx->scene.controller.velocity) <= 0.5)
      ctx->scene.controller.velocity = (float3){0};
  } else {
    ctx->scene.controller.velocity = float3_scale(ctx->scene.controller.velocity, 0.98f);
  }

  // update position
  ctx->scene.camera_pos.position = float3_add(ctx->scene.camera_pos.position, float3_scale(ctx->scene.controller.velocity, dt));

  // snap
  float ground = get_interpolated_terrain_height(ctx->scene.camera_pos.position.x, ctx->scene.camera_pos.position.z);
  ctx->scene.camera_pos.position.y = ground + ctx->scene.controller.camera_height_offset;

  // mouse input
  float mx, my;
  SDL_GetRelativeMouseState(&mx, &my);

  ctx->scene.camera_pos.yaw += mx * ctx->scene.controller.mouse_sensitivity;
  ctx->scene.camera_pos.pitch -= my * ctx->scene.controller.mouse_sensitivity;

  if (ctx->scene.camera_pos.pitch < ctx->scene.controller.min_pitch) ctx->scene.camera_pos.pitch = ctx->scene.controller.min_pitch;
  if (ctx->scene.camera_pos.pitch > ctx->scene.controller.max_pitch) ctx->scene.camera_pos.pitch = ctx->scene.controller.max_pitch;

  ctx->scene.controller.ground_height = ground;
  update_camera(&ctx->renderer, &ctx->scene.camera_pos);
}

static void on_generate(void *args, size_t size) {
  (void)size; // unused
  struct context_t *ctx = (struct context_t *)args;

  if (!ctx) return;

  ctx->scene = (scene_t) {
    .camera_pos = {
      .pitch = -0.3f,  // Look down slightly to see the terrain
      .yaw = 0.0f,
      .position = {0, 0, 0}
    },
    .chunk_map = { 0 },
    .controller = (fps_controller_t) {
      .delta_time = 1.0f / g_world_config.tick_rate,
      .last_frame_time = 0.0f,
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

  // Update camera with current position
  update_camera(&ctx->renderer, &ctx->scene.camera_pos);
}

static void on_normal_tick(void *args, size_t size, float dt) {
  (void)size; // unused
  if (!args) return;

  struct context_t *ctx = (struct context_t*)args;

  if (ctx->keys[SDL_SCANCODE_Q]) {
    ctx->scene.controller.skiing = !ctx->scene.controller.skiing;
    ctx->scene.controller.velocity = make_float3(0.0f, 0.0f, 0.0f);
  }

  // apply movement
  if (ctx->scene.controller.skiing)   apply_ski_movement(ctx, dt);
  else                                apply_fps_movement(ctx, dt);

  // bob camera
  // float camera_bob_offset = sinf(ctx->scene.controller.distance_walked);
  ctx->scene.camera_pos.position.y = ctx->scene.controller.ground_height + ctx->scene.controller.camera_height_offset;// + camera_bob_offset;

  // update loaded chunks
  update_loaded_chunks(&ctx->scene);

  ctx->scene.sun.color = get_sun_color(ctx->total_time);
}

static int on_normal_render(void *args, size_t size) {
  (void)size; // unused
  if (!args) return 0;

  struct context_t *ctx = (struct context_t*)args;

  usize triangles_rendered = render_loaded_chunks(&ctx->renderer, &ctx->scene, &ctx->scene.sun, 1);

  u8 fog_r, fog_g, fog_b;
  get_fog_color(ctx->total_time, &fog_r, &fog_g, &fog_b);
  apply_fog_to_screen(&ctx->renderer, ctx->renderer.max_depth * ctx->scene.fog_start, ctx->renderer.max_depth, fog_r, fog_g, fog_b);

  return triangles_rendered;
}

static void on_overhead_enter(void *args, size_t size) {
  (void)size; // unused
  if (!args) return;

  struct context_t *ctx = (struct context_t*)args;

  ctx->scene.camera_pos.position.y += 250.0f;
  ctx->scene.camera_pos.pitch = -PI / 2;
  ctx->scene.camera_pos.yaw = 0;

  ctx->renderer.max_depth = 500;
}

static void on_overhead_tick(void *args, size_t size, float dt) {
  (void)size; // unused
  if (!args) return;

  struct context_t *ctx = (struct context_t*)args;

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

  float TICK_INTERVAL = 1.0f / g_world_config.tick_rate;

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

        if (event.key.scancode == SDL_SCANCODE_M) {
          if (fsm_get_state(&sm) != OVERHEAD)
            fsm_change_state(&sm, OVERHEAD);
          else
            fsm_change_state(&sm, NORMAL);
        }

        if (event.key.key == SDLK_1) {
          renderer.wireframe_mode = !renderer.wireframe_mode;
        }
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

    for(size_t i = 0; i < config_width * config_height; ++i) {
      framebuffer[i] = background_color;
      depth_buffer[i] = FLT_MAX;
    }

    int triangles_rendered = fsm_render_state(&sm);

    SDL_UpdateTexture(sdl_framebuff, NULL, framebuffer, config_width * sizeof(u32));
    SDL_RenderTexture(sdl_renderer, sdl_framebuff, NULL, NULL);
    SDL_RenderPresent(sdl_renderer);

    stats.fps_counter++;
    stats.triangle_counter += triangles_rendered;
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
