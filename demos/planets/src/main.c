#include <SDL3/SDL.h>

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include <shader-works/renderer.h>
#include <shader-works/maths.h>
#include <shader-works/primitives.h>
#include "../common/noise.h"


#define WIN_WIDTH 150
#define WIN_HEIGHT 100
#define WIN_SCALE 8
#define WIN_TITLE "Planets Demo - Press SPACE for new planet"
#define MAX_DEPTH 15
#define TEXTURE_SIZE 200

typedef struct {
  model_t model;
  float radius, rotation_speed;
  u32 *surface_texture, *background_texture;
  float3 sun_color, sun_direction;
  fragment_shader_t frag_shader;
} planet_t;

// Color conversion functions using SDL (required by shader-works)
u32 rgb_to_u32(u8 r, u8 g, u8 b) {
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);
  return SDL_MapRGBA(format, NULL, r, g, b, 255);
}

void u32_to_rgb(u32 color, u8 *r, u8 *g, u8 *b) {
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);
  SDL_GetRGB(color, format, NULL, r, g, b);
}

float rand_float_range(float min, float max) {
  float scale = rand() / (float) RAND_MAX; // [0, 1.0]
  return min + scale * (max - min); // [min, max]
}

float normalize_to_range(float normalized, float min, float max) {
  return min + normalized * (max - min);
}

u32 planet_frag_func(u32 input, fragment_context_t *ctx, void *args, usize argc) {
  (void)input; (void)argc;

  if (!args || !ctx->light || ctx->light_count == 0) {
    return rgb_to_u32(100, 100, 100); // fallback color if no lighting info
  }

  // Simple Lambertian diffuse shading with the first light source
  light_t *light = &ctx->light[0];

  // Calculate diffuse intensity
  float3 light_dir = light->is_directional ? light->direction : (float3){
    light->position.x - ctx->world_pos.x,
    light->position.y - ctx->world_pos.y,
    light->position.z - ctx->world_pos.z
  };

  // Normalize light direction
  float len = sqrtf(light_dir.x * light_dir.x + light_dir.y * light_dir.y + light_dir.z * light_dir.z);
  if (len > 0.0001f) {
    light_dir.x /= len;
    light_dir.y /= len;
    light_dir.z /= len;
  }

  // Diffuse intensity based on angle between normal and light direction
  float diffuse = fmaxf(0.0f, ctx->normal.x * light_dir.x + ctx->normal.y * light_dir.y + ctx->normal.z * light_dir.z);

  // Sample texture color
  int tex_x = (int)(ctx->uv.x * TEXTURE_SIZE) % TEXTURE_SIZE;
  int tex_y = (int)(ctx->uv.y * TEXTURE_SIZE) % TEXTURE_SIZE;
  u32 tex_color = ((u32*)args)[tex_y * TEXTURE_SIZE + tex_x];

  // Apply lighting to texture color
  u8 r, g, b;
  u32_to_rgb(tex_color, &r, &g, &b);

  r = (u8)(r * diffuse * ((light->color >> 16) & 0xFF) / 255.0f);
  g = (u8)(g * diffuse * ((light->color >> 8) & 0xFF) / 255.0f);
  b = (u8)(b * diffuse * (light->color & 0xFF) / 255.0f);

  return rgb_to_u32(r, g, b);
}

void generate_background_texture(planet_t *p, int seed) {
  // Free old texture if it exists
  if (p->background_texture) {
    free(p->background_texture);
  }

  // Create new texture and fill with Perlin noise
  p->background_texture = (u32 *)malloc(WIN_WIDTH * WIN_HEIGHT * sizeof(u32));

  for (int y = 0; y < WIN_HEIGHT; y++) {
    for (int x = 0; x < WIN_WIDTH; x++) {
      u8 brightness;
      float star_mask = ridgeNoise((float)x / WIN_WIDTH * 10.0f, (float)y / WIN_HEIGHT * 10.0f, seed);

      if (star_mask > 0.995f) {
        float distance = rand_float_range(0.0f, 1.0f);

        // Close stars are white, distant stars shift to red
        if (distance < 0.5f) {
          // Close stars - bright white
          brightness = (u8)normalize_to_range(distance, 255, 150); // Closer to 0 = brighter
          p->background_texture[y * WIN_WIDTH + x] = rgb_to_u32(brightness, brightness, brightness);
        } else {
          // Distant stars - shift from white to red based on distance
          float red_amount = normalize_to_range(distance - 0.3f, 0.3f, 1.0f); // Closer to 1 = more red
          brightness = 80;  // Much dimmer for distant stars
          u8 red = (u8)(brightness * 0.6f);  // Reduce brightness further
          u8 green = (u8)(brightness * (1.0f - red_amount) * 0.3f);
          u8 blue = (u8)(brightness * (1.0f - red_amount) * 0.3f);
          p->background_texture[y * WIN_WIDTH + x] = rgb_to_u32(red, green, blue);
        }
        continue;
      }

      float noise_val = fbm((float)x / WIN_WIDTH * 4.0f, (float)y / WIN_HEIGHT * 4.0f, 5, seed);
      noise_val = fmaxf(0.0f, fminf(1.0f, noise_val));

      brightness = (u8)(noise_val * 50); // Dark purple background

      p->background_texture[y * WIN_WIDTH + x] = rgb_to_u32(brightness, 0, brightness * 1.75f);
    }
  }

  // sun color, direction
  p->sun_color = make_float3(255, rand_float_range(60, 255), rand_float_range(60, 255));
  p->sun_direction = make_float3(-rand_float_range(0.1f, 0.9f), -rand_float_range(0.1f, 0.9f), -rand_float_range(0.1f, 0.9f));
}

void generate_surface_texture(planet_t *p, int seed) {
  // Free old texture if it exists
  if (p->surface_texture) {
    free(p->surface_texture);
  }

  // Create new texture and fill with Perlin noise
  p->surface_texture = (u32 *)malloc(TEXTURE_SIZE * TEXTURE_SIZE * sizeof(u32));

  for (int y = 0; y < TEXTURE_SIZE; y++) {
    for (int x = 0; x < TEXTURE_SIZE; x++) {
      // Use polar coordinates for horizontal wrapping
      float angle = (float)x / TEXTURE_SIZE * 2.0f * M_PI;
      float x_coord = cosf(angle) * 1.5f;
      float y_coord = sinf(angle) * 1.5f + (float)y / TEXTURE_SIZE * 4.0f;

      float fbm_val = fbm(x_coord, y_coord, 5, seed);
      float ridge_val = ridgeNoise(x_coord, y_coord, seed + 1000);

      float noise_val = pow(fbm_val * 0.5f + ridge_val * 0.5f, 1.2f) - 0.5f;

      float normalized = (noise_val + 1.0f) * 0.5f;
      normalized = fmaxf(0.0f, fminf(1.0f, normalized));

      normalized = powf(normalized, 0.95f);

      u32 color_val = rgb_to_u32(0, 0, 100);
      if (normalized < 0.3f) {
        color_val = rgb_to_u32(0, 0, (u8)normalize_to_range(normalized, 50, 150));
      } else if (normalized < 0.4f) {
        color_val = rgb_to_u32(0, (u8)normalize_to_range(normalized - 0.3f, 100, 255), 200);
      } else if (normalized < 0.5f) {
        color_val = rgb_to_u32(200, 200, 100);
      } else if (normalized < 0.6f) {
        color_val = rgb_to_u32(0, (u8)normalize_to_range(normalized - 0.5f, 150, 255), 0);
      } else if (normalized < 0.8f) {
        u8 val = (u8)normalize_to_range(normalized - 0.6f, 70, 150);
        color_val = rgb_to_u32(val, val, val);
      } else {
        color_val = rgb_to_u32(255, 255, 255);
      }

      p->surface_texture[y * TEXTURE_SIZE + x] = color_val;
    }
  }
}

void new_planet(planet_t *p) {
  if (!&p->model) return;

  int seed = time(NULL);
  srand(seed);

  p->radius = rand_float_range(1.75f, 2.5f);
  p->rotation_speed = rand_float_range(0.0001f, 0.0025f);
  p->model.transform.pitch = rand_float_range(0.0f, 1.5f * M_PI);

  generate_sphere(&p->model, p->radius, 96, 96, make_float3(0.0f, 0.f, -8.0f));

  // Free old textures if they exist
  if (p->surface_texture) {
    free(p->surface_texture);
  }
  if (p->background_texture) {
    free(p->background_texture);
  }

  // Create 100x100 textures and fill with Perlin noise
  int texture_area = TEXTURE_SIZE * TEXTURE_SIZE;
  p->surface_texture = (u32 *)malloc(texture_area * sizeof(u32));
  p->background_texture = (u32 *)malloc(WIN_WIDTH * WIN_HEIGHT * sizeof(u32));

  generate_surface_texture(p, seed);
  generate_background_texture(p, seed);

  // Tell the renderer how to shade the model
  p->frag_shader = make_fragment_shader(planet_frag_func, p->surface_texture, 1);
  p->model.frag_shader = &p->frag_shader;  // Point to the shader struct in planet_t
  p->model.vertex_shader = &default_vertex_shader;      // Use default vertex shader
  p->model.use_textures = true;                         // Use texture with lighting
}

void sdl_setup(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **framebuffer_tex) {
  // Initialize SDL and create window/renderer
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(WIN_TITLE, WIN_WIDTH * WIN_SCALE, WIN_HEIGHT * WIN_SCALE, 0, window, renderer);

  // Create framebuffer texture with nearest-neighbor scaling
  *framebuffer_tex = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIN_WIDTH, WIN_HEIGHT);
  SDL_SetTextureScaleMode(*framebuffer_tex, SDL_SCALEMODE_NEAREST);
}

int main(int argc, char *argv[]) {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture* framebuffer_tex;

  // Rendering buffers, used by shader-works
  u32 framebuffer[WIN_WIDTH * WIN_HEIGHT];
  f32 depthbuffer[WIN_WIDTH * WIN_HEIGHT];

  renderer_t renderer_state = { 0 };
  bool running = true;

  // Initialize SDL and create window/renderer
  sdl_setup(&window, &renderer, &framebuffer_tex);

  // Initialize shader-works renderer
  init_renderer(&renderer_state, WIN_WIDTH, WIN_HEIGHT, 0, 0, framebuffer, depthbuffer, NULL, MAX_DEPTH);

  // Initialize game objects
  planet_t sphere_model = {0};
  new_planet(&sphere_model);

  transform_t camera = {0};                               // cameras are just transforms
  camera.position = make_float3(0.0f, 0.0f, 0.0f);

  light_t sun = {
    .is_directional = true,
    .direction = sphere_model.sun_direction,
    .color = rgb_to_u32((u8)sphere_model.sun_color.x, (u8)sphere_model.sun_color.y, (u8)sphere_model.sun_color.z)
  };

  update_camera(&renderer_state, &camera);

  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }

      if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.key == SDLK_SPACE) {
          new_planet(&sphere_model);

          sun.direction = sphere_model.sun_direction;
          sun.color = rgb_to_u32((u8)sphere_model.sun_color.x, (u8)sphere_model.sun_color.y, (u8)sphere_model.sun_color.z);
        }

        if (event.key.key == SDLK_ESCAPE) {
          running = false;
        }
      }
    }

    // rotate sphere
    sphere_model.model.transform.yaw += sphere_model.rotation_speed;

    // Clear framebuffer and reset depth buffer
    // Fill background with procedural texture
    for (int i = 0; i < WIN_WIDTH * WIN_HEIGHT; i++) {
      framebuffer[i] = sphere_model.background_texture[i];
      depthbuffer[i] = MAX_DEPTH;
    }

    // render model
    render_model(&renderer_state, &camera, &sphere_model.model, &sun, 1);

    // Present framebuffer to screen
    SDL_UpdateTexture(framebuffer_tex, NULL, framebuffer, WIN_WIDTH * sizeof(u32));
    SDL_RenderTexture(renderer, framebuffer_tex, NULL, NULL);
    SDL_RenderPresent(renderer);

    SDL_Delay(1); // Small delay to prevent 100% CPU usage
  }

  // Cleanup resources
  delete_model(&sphere_model.model);
  if (sphere_model.surface_texture) {
    free(sphere_model.surface_texture);
  }

  if (sphere_model.background_texture) {
    free(sphere_model.background_texture);
  }

  SDL_DestroyTexture(framebuffer_tex);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();
  return 0;
}
