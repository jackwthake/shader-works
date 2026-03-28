#include <shader-works/renderer.h>
#include <shader-works/primitives.h>

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
  u32 *skybox_buffer;

  renderer_t renderer;
  scene_t scene;
  const bool *keys;

  model_t skybox_sphere;
  skybox_shader_args_t skybox_shader_args;

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
  float3 normal = get_slope_normal(ctx->scene.camera_pos.position.x, ctx->scene.camera_pos.position.z, g_world_config.seed);
  float3 gravity = { 0.0f, -24.f, 0.0f };
  float slope_dot = float3_dot(gravity, normal);

  float3 downhill_acc = float3_sub(gravity, float3_scale(normal, slope_dot));
  float3 kick_force = {0};

  if (ctx->scene.controller.kick_timer > 0.0f) {
    ctx->scene.controller.kick_timer -= dt;
  }

  if (ctx->scene.controller.side_step_timer > 0.0f) {
    ctx->scene.controller.side_step_timer -= dt;
  }

  // mouse input
  float mx, my;
  SDL_GetRelativeMouseState(&mx, &my);

  ctx->scene.camera_pos.yaw += mx * ctx->scene.controller.mouse_sensitivity;
  ctx->scene.camera_pos.pitch -= my * ctx->scene.controller.mouse_sensitivity;

  if (ctx->scene.camera_pos.pitch < ctx->scene.controller.min_pitch) ctx->scene.camera_pos.pitch = ctx->scene.controller.min_pitch;
  if (ctx->scene.camera_pos.pitch > ctx->scene.controller.max_pitch) ctx->scene.camera_pos.pitch = ctx->scene.controller.max_pitch;

  float ground = get_interpolated_terrain_height(ctx->scene.camera_pos.position.x, ctx->scene.camera_pos.position.z);
  ctx->scene.camera_pos.position.y = ground + ctx->scene.controller.camera_height_offset;
  ctx->scene.controller.ground_height = ground;
  update_camera(&ctx->renderer, &ctx->scene.camera_pos);

  float3 right, up, forward;
  transform_get_basis_vectors(&ctx->scene.camera_pos, &right, &up, &forward);

  const bool *keys = ctx->keys;
  if (keys[SDL_SCANCODE_W] && ctx->scene.controller.kick_timer <= 0.0f) {
    kick_force = float3_scale(forward, -115.f);

    ctx->scene.controller.kick_timer = ctx->scene.controller.kick_interval;
  }

  // apply forces
  ctx->scene.controller.velocity = float3_add(ctx->scene.controller.velocity, float3_scale(downhill_acc, dt));
  ctx->scene.controller.velocity = float3_add(ctx->scene.controller.velocity, float3_scale(kick_force, dt));

  // steer
  float3 vel = ctx->scene.controller.velocity;
  float speed = float3_magnitude(vel);

  if (speed > 1.0f) {
    float3 vel_dir = float3_normalize(vel);
    float3 target_dir = float3_scale(forward, -1.0f);
    target_dir.y = 0.0f;
    target_dir = float3_normalize(target_dir);

    float alignment = float3_dot(vel_dir, target_dir);

    if (alignment < 0.98f) {
      float3 steer_delta = float3_sub(target_dir, vel_dir);

      float grip = 12.0f;
      float3 grip_force = float3_scale(steer_delta, grip * speed * dt);
      float3 turn_scrub_factor = float3_scale(grip_force, -0.15f);

      ctx->scene.controller.velocity = float3_add(ctx->scene.controller.velocity, grip_force);
      ctx->scene.controller.velocity = float3_add(ctx->scene.controller.velocity, turn_scrub_factor);
    }
  }

  // friction
  if (keys[SDL_SCANCODE_S]) {
    ctx->scene.controller.velocity = float3_scale(ctx->scene.controller.velocity, 0.70f);

    if (float3_magnitude(ctx->scene.controller.velocity) <= 1.0f)
      ctx->scene.controller.velocity = (float3){0};
  } else {
    ctx->scene.controller.velocity = float3_scale(ctx->scene.controller.velocity, 0.975f);
  }

  if (float3_magnitude(ctx->scene.controller.velocity) <= 0.5f)
    ctx->scene.controller.velocity = (float3){0};

  // update position
  ctx->scene.camera_pos.position = float3_add(ctx->scene.camera_pos.position, float3_scale(ctx->scene.controller.velocity, dt));

  if (float3_magnitude(ctx->scene.controller.velocity) <= 2.0f) {
    if (ctx->scene.controller.side_step_timer <= 0.0f) {
      if (keys[SDL_SCANCODE_A]) ctx->scene.camera_pos.position = float3_add(ctx->scene.camera_pos.position, float3_scale(right, 2.f));
      if (keys[SDL_SCANCODE_D]) ctx->scene.camera_pos.position = float3_add(ctx->scene.camera_pos.position, float3_scale(right, -2.f));

      ctx->scene.controller.side_step_timer = ctx->scene.controller.kick_interval / 3.f;
    }
  }

  // snap
  ctx->scene.camera_pos.position.y = ground + ctx->scene.controller.camera_height_offset;
  ctx->scene.controller.ground_height = ground;
  update_camera(&ctx->renderer, &ctx->scene.camera_pos);
}

static void regenerate_skybox_sphere(struct context_t *ctx) {
  if (ctx->skybox_sphere.vertex_data) free(ctx->skybox_sphere.vertex_data);
  if (ctx->skybox_sphere.face_normals) free(ctx->skybox_sphere.face_normals);
  if (ctx->skybox_sphere.frag_shader) free(ctx->skybox_sphere.frag_shader);
  ctx->skybox_sphere = (model_t){0};

  f32 skybox_radius = ctx->renderer.max_depth * 0.95f;
  generate_sphere(&ctx->skybox_sphere, skybox_radius, 32, 16, make_float3(0, 0, 0));

  ctx->skybox_sphere.use_textures = true;  // CRITICAL: Enable UV interpolation
  ctx->skybox_sphere.disable_behind_camera_culling = true; // Always render skybox triangles, even if "facing away" from camera
  ctx->skybox_sphere.transform.pitch = M_PI / 2; // Rotate so that texture aligns correctly with world axes

  // Setup skybox shader
  ctx->skybox_sphere.frag_shader = malloc(sizeof(fragment_shader_t));
  if (ctx->skybox_sphere.frag_shader) {
    *ctx->skybox_sphere.frag_shader = make_fragment_shader(skybox_frag_shader_func, &ctx->skybox_shader_args, sizeof(skybox_shader_args_t));
  }
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

  // Regenerate skybox with new max_depth
  regenerate_skybox_sphere(ctx);

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
    ctx->scene.controller.kick_timer = ctx->scene.controller.kick_interval;
  }

  // apply movement
  if (ctx->scene.controller.skiing)   apply_ski_movement(ctx, dt);
  else                                apply_fps_movement(ctx, dt);

  ctx->scene.camera_pos.position.y = ctx->scene.controller.ground_height + ctx->scene.controller.camera_height_offset;// + camera_bob_offset;

  // update loaded chunks
  update_loaded_chunks(&ctx->scene);

  ctx->scene.sun.color = get_sun_color(ctx->total_time);
  ctx->skybox_sphere.transform.position = ctx->scene.camera_pos.position; // Keep skybox centered on camera
}

static int on_normal_render(void *args, size_t size) {
  (void)size; // unused
  if (!args) return 0;

  struct context_t *ctx = (struct context_t*)args;

  // Render skybox sphere first (centered at camera)
  transform_t skybox_transform = ctx->scene.camera_pos;
  usize triangles_rendered = render_model(&ctx->renderer, &skybox_transform, &ctx->skybox_sphere, &ctx->scene.sun, 1);

  // Render scene geometry (will overdraw skybox via depth test)
  triangles_rendered += render_loaded_chunks(&ctx->renderer, &ctx->scene, &ctx->scene.sun, 1);

  u8 fog_r, fog_g, fog_b;
  get_fog_color(ctx->total_time, &fog_r, &fog_g, &fog_b);
  apply_fog_to_screen(&ctx->renderer, ctx->scene.fog_start, ctx->renderer.max_depth, fog_r, fog_g, fog_b);

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

  // Regenerate skybox with new max_depth
  regenerate_skybox_sphere(ctx);
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

// Regenerate skybox texture with time-based noise evolution
void regenerate_skybox_texture(u32 *skybox_buffer, u32 width, u32 height, int base_seed, float time_offset) {
  for (unsigned int y = 0; y < height; y++) {
    for (unsigned int x = 0; x < width; x++) {
      // Normalize coordinates to 0-1 range for noise sampling
      float u = (float)x / width;
      float v = (float)y / height;

      // Sample Perlin noise with time-based offset for animation
      // Layer multiple octaves at different scales for sharp cloud definition
      float noise_val = noise2D(u * 24.0f + time_offset * 0.3f,
                                v * 24.0f + time_offset * 0.2f,
                                base_seed);
      noise_val += noise2D(u * 33.0f - time_offset * 0.15f,
                          v * 33.0f - time_offset * 0.1f,
                          base_seed + 100) * 0.4f;
      noise_val += noise2D(u * 5.0f + time_offset * 0.1f,
                          v * 5.0f + time_offset * 0.15f,
                          base_seed + 200) * 0.3f;

      // Map noise from [-1, 1] to [0, 1]
      float cloud_density = fminf(fmaxf((noise_val + 1.0f) * 0.5f, 0.0f), 1.0f);

      // Create gradient blend: mostly blue with transparent cloud effect
      // Clouds only become visible above 0.4, fully white at 1.0
      float blend = 0.0f;
      if (cloud_density > 0.4f) {
        blend = (cloud_density - 0.4f) / 0.6f;  // Maps 0.4-1.0 to 0-1
        if (blend > 1.0f) blend = 1.0f;
      }

      // Interpolate between sky blue and white
      u8 r = (u8)(135.0f + (255.0f - 155.0f) * blend);
      u8 g = (u8)(206.0f + (255.0f - 226.0f) * blend);
      u8 b = (u8)(235.0f + (255.0f - 255.0f) * blend);
      u32 color = rgb_to_u32(r, g, b);
      skybox_buffer[y * width + x] = color;
    }
  }
}

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
  u32 *skybox_buffer = (u32 *)malloc((config_width / 0.5) * (config_height / 0.5) * sizeof(u32));

  // Initialize state and window
  SDL_library_init(&sdl_window, &sdl_renderer, &sdl_framebuff, config_title, config_width, config_height, config_scale);
  SDL_SetWindowRelativeMouseMode(sdl_window, true);

  renderer_t renderer = {0};
  init_renderer(&renderer, config_width, config_height, 0, 0, framebuffer, depth_buffer, skybox_buffer, MAX_DEPTH);

  // Initialize skybox texture with current time
  regenerate_skybox_texture(skybox_buffer, config_width, config_height, g_world_config.seed, 0.0f);

  performance_counter stats;
  init_performance_counter(&stats);

  float TICK_INTERVAL = 1.0f / g_world_config.tick_rate;

  // Initialize state machine
  state_machine_t sm = {0};
  fsm_init(&sm, GENERATE, NUM_STATES);

  // Create skybox shader args (will be used by regenerate_skybox_sphere)
  skybox_shader_args_t skybox_args = {
    .skybox_buffer = skybox_buffer,
    .width = config_width,
    .height = config_height
  };

  struct context_t state_context = {
    .framebuffer = framebuffer,
    .depth_buffer = depth_buffer,
    .skybox_buffer = skybox_buffer,
    .renderer = renderer,
    .skybox_sphere = {0},
    .skybox_shader_args = skybox_args,

    .keys = SDL_GetKeyboardState(NULL),
    .sm = &sm,
    .total_time = 0.0f,
  };

  // Generate initial skybox sphere
  regenerate_skybox_sphere(&state_context);

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

    u32 background_color = rgb_to_u32(0, 0, 0);

    for(size_t i = 0; i < config_width * config_height; ++i) {
      framebuffer[i] = background_color;
      depth_buffer[i] = FLT_MAX;
    }

    int triangles_rendered = fsm_render_state(&sm);

    // Regenerate skybox texture every frame for animation
    regenerate_skybox_texture(skybox_buffer, config_width, config_height, g_world_config.seed, state_context.total_time);

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

  // Cleanup skybox
  if (state_context.skybox_sphere.vertex_data) free(state_context.skybox_sphere.vertex_data);
  if (state_context.skybox_sphere.face_normals) free(state_context.skybox_sphere.face_normals);
  if (state_context.skybox_sphere.frag_shader) free(state_context.skybox_sphere.frag_shader);

  free(framebuffer);
  free(depth_buffer);
  free(skybox_buffer);

  SDL_DestroyTexture(sdl_framebuff);
  SDL_DestroyRenderer(sdl_renderer);
  SDL_DestroyWindow(sdl_window);
  SDL_Quit();

  free_config();

  return 0;
}
