#include <float.h>
#include <math.h>
#include <stdbool.h>

#include <shader-works/renderer.h>
#include <shader-works/primitives.h>
#include <shader-works/shaders.h>

#include <SDL3/SDL.h>

#define WIN_WIDTH 400
#define WIN_HEIGHT 250
#define WIN_SCALE 4
#define WIN_TITLE "MicroCraft"
#define MAX_DEPTH 15

// PLACEHOLDER

typedef struct {
  float move_speed;
  float mouse_sensitivity;
  float min_pitch, max_pitch;
  float ground_height;
  float delta_time;
  uint64_t last_frame_time;
} fps_controller_t;

u32 rgb_to_u32(u8 r, u8 g, u8 b) {
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);
  return SDL_MapRGBA(format, NULL, r, g, b, 255);
}

void u32_to_rgb(u32 color, u8 *r, u8 *g, u8 *b) {
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);
  SDL_GetRGB(color, format, NULL, r, g, b);
}

u32 soft_red_shader_func(u32 input, fragment_context_t *ctx, void *args, usize argc) {
  (void)input; (void)ctx; (void)args; (void)argc;
  return default_lighting_frag_shader.func(rgb_to_u32(255, 100, 100), ctx, args, argc);
}

u32 soft_green_shader_func(u32 input, fragment_context_t *ctx, void *args, usize argc) {
  (void)input; (void)ctx; (void)args; (void)argc;
  return default_lighting_frag_shader.func(rgb_to_u32(100, 255, 100), ctx, args, argc);
}

void update_timing(fps_controller_t *controller) {
  uint64_t current_time = SDL_GetPerformanceCounter();
  controller->delta_time = (float)(current_time - controller->last_frame_time) / SDL_GetPerformanceFrequency();
  controller->last_frame_time = current_time;
  if (controller->delta_time > 0.1f) controller->delta_time = 0.1f;
}

void handle_input(fps_controller_t *controller, transform_t *camera, bool *mouse_captured) {
  const bool *keys = SDL_GetKeyboardState(NULL);

  // Mouse input
  if (*mouse_captured) {
    float mx, my;
    SDL_GetRelativeMouseState(&mx, &my);
    camera->yaw += mx * controller->mouse_sensitivity;
    camera->pitch -= my * controller->mouse_sensitivity;
    if (camera->pitch < controller->min_pitch) camera->pitch = controller->min_pitch;
    if (camera->pitch > controller->max_pitch) camera->pitch = controller->max_pitch;
  }

  // Keyboard movement
  float cy = cosf(camera->yaw), sy = sinf(camera->yaw);
  float cp = cosf(camera->pitch), sp = sinf(camera->pitch);
  float3 right = make_float3(cy, 0, -sy);
  float3 forward = make_float3(-sy * cp, sp, -cy * cp);
  float3 movement = make_float3(0, 0, 0);
  float speed = controller->move_speed * controller->delta_time;

  if (keys[SDL_SCANCODE_W]) movement = float3_add(movement, float3_scale(forward, speed));
  if (keys[SDL_SCANCODE_S]) movement = float3_add(movement, float3_scale(forward, -speed));
  if (keys[SDL_SCANCODE_A]) movement = float3_add(movement, float3_scale(right, speed));
  if (keys[SDL_SCANCODE_D]) movement = float3_add(movement, float3_scale(right, -speed));

  camera->position = float3_add(camera->position, movement);
  camera->position.y = controller->ground_height;
}

int main(int argc, char *argv[]) {
  SDL_Init(SDL_INIT_VIDEO);

  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_CreateWindowAndRenderer(WIN_TITLE, WIN_WIDTH * WIN_SCALE, WIN_HEIGHT * WIN_SCALE, 0, &window, &renderer);

  SDL_Texture *framebuffer_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIN_WIDTH, WIN_HEIGHT);
  SDL_SetTextureScaleMode(framebuffer_tex, SDL_SCALEMODE_NEAREST);

  u32 framebuffer[WIN_WIDTH * WIN_HEIGHT];
  f32 depthbuffer[WIN_WIDTH * WIN_HEIGHT];

  fps_controller_t controller = {
    .move_speed = 8.0f,
    .mouse_sensitivity = 0.0015f,
    .min_pitch = -M_PI/2 + 0.1f,
    .max_pitch = M_PI/2 - 0.1f,
    .ground_height = 2.0f,
    .last_frame_time = SDL_GetPerformanceCounter()
  };

  transform_t camera = { .position = make_float3(0, 2, 0) };
  bool mouse_captured = true;
  bool running = true;

  SDL_SetWindowRelativeMouseMode(window, true);

  renderer_t renderer_state = {0};
  init_renderer(&renderer_state, WIN_WIDTH, WIN_HEIGHT, 0, 0, framebuffer, depthbuffer, MAX_DEPTH);

  fragment_shader_t soft_red_shader = make_fragment_shader(soft_red_shader_func, NULL, 0);
  fragment_shader_t soft_green_shader = make_fragment_shader(soft_green_shader_func, NULL, 0);

  model_t ground = {0}, cube = {0};
  generate_plane(&ground, (float2){20, 20}, (float2){1, 1}, (float3){0, 0, 0});
  generate_cube(&cube, make_float3(0, 2, -6), make_float3(1, 1, 1));

  cube.frag_shader = &soft_red_shader;
  ground.frag_shader = &soft_green_shader;
  ground.vertex_shader = cube.vertex_shader = &default_vertex_shader;
  ground.use_textures = cube.use_textures = false;

  cube.transform.yaw = M_PI/4;
  cube.transform.pitch = M_PI/8;

  light_t sun = {
    .is_directional = true,
    .direction = make_float3(-1, -1, -1),
    .color = rgb_to_u32(255, 255, 255)
  };

  while (running) {
    update_timing(&controller);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) running = false;
      if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
        mouse_captured = !mouse_captured;
        SDL_SetWindowRelativeMouseMode(window, mouse_captured);
      }
    }

    handle_input(&controller, &camera, &mouse_captured);
    update_camera(&renderer_state, &camera);

    for(int i = 0; i < WIN_WIDTH * WIN_HEIGHT; ++i) {
      framebuffer[i] = rgb_to_u32(50, 50, 175);
      depthbuffer[i] = FLT_MAX;
    }

    render_model(&renderer_state, &camera, &ground, &sun, 1);
    render_model(&renderer_state, &camera, &cube, &sun, 1);

    SDL_UpdateTexture(framebuffer_tex, NULL, framebuffer, WIN_WIDTH * sizeof(u32));
    SDL_RenderTexture(renderer, framebuffer_tex, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  delete_model(&ground);
  delete_model(&cube);
  SDL_DestroyTexture(framebuffer_tex);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
