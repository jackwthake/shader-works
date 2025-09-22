#include <SDL3/SDL.h>

#include <float.h>
#include <math.h>

#include <cpu-render/renderer.h>
#include <cpu-render/maths.h>
#include <cpu-render/primitives.h>

#define WIN_WIDTH 400
#define WIN_HEIGHT 250
#define WIN_SCALE 4
#define WIN_TITLE "Basic Demo"
#define MAX_DEPTH 15

int main(int argc, char *argv[]) {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture* framebuffer_tex;

  // Rendering buffers, used by cpu-render
  u32 framebuffer[WIN_WIDTH * WIN_HEIGHT];
  f32 depthbuffer[WIN_WIDTH * WIN_HEIGHT];

  renderer_t renderer_state = { 0 };
  bool running = true;
  
  // Initialize SDL and create window/renderer
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(WIN_TITLE, WIN_WIDTH * WIN_SCALE, WIN_HEIGHT * WIN_SCALE, 0, &window, &renderer);

  // Create framebuffer texture with nearest-neighbor scaling
  framebuffer_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIN_WIDTH, WIN_HEIGHT);
  SDL_SetTextureScaleMode(framebuffer_tex, SDL_SCALEMODE_NEAREST);

  // Initialize CPU renderer
  init_renderer(&renderer_state, WIN_WIDTH, WIN_HEIGHT, 0, 0, framebuffer, depthbuffer, MAX_DEPTH);

  // Initialize game objects
  model_t cube_model = {0};
  generate_cube(&cube_model, make_float3(0.0f, 2.0f, -6.0f), make_float3(1.0f, 1.0f, 1.0f));

  // Tell the renderer how to shade the model
  cube_model.frag_shader = &default_lighting_frag_shader; // apply our directional light
  cube_model.vertex_shader = &default_vertex_shader;      // Use default vertex shader
  cube_model.use_textures = false;                        // Use flat shading
  
  transform_t camera = {0};                               // cameras are just transforms
  camera.position = make_float3(0.0f, 2.0f, 0.0f);

  light_t sun = {
    .is_directional = true,
    .direction = make_float3(-1, -1, -1),
    .color = rgb_to_u32(255, 255, 255)
  };

  update_camera(&renderer_state, &camera);

  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }

    // rotate cube
    cube_model.transform.yaw  += 1.0f * 0.025;
    cube_model.transform.pitch += 0.5f * 0.025;
    
    // slowly move cube back and forth
    cube_model.transform.position.z = ((sinf(SDL_GetTicks() / 1000.0f) * 4.f) - 7.0f);

    // Clear framebuffer and reset depth buffer
    for(int i = 0; i < WIN_WIDTH * WIN_HEIGHT; ++i) {
      framebuffer[i] = rgb_to_u32(100, 100, 255);
      depthbuffer[i] = FLT_MAX;
    }

    // render model
    render_model(&renderer_state, &camera, &cube_model, &sun, 1);

    // Present framebuffer to screen
    SDL_UpdateTexture(framebuffer_tex, NULL, framebuffer, WIN_WIDTH * sizeof(u32));
    SDL_RenderTexture(renderer, framebuffer_tex, NULL, NULL);
    SDL_RenderPresent(renderer);
    
    SDL_Delay(1); // Small delay to prevent 100% CPU usage
  }
  
  // Cleanup resources
  delete_model(&cube_model);
  
  SDL_DestroyTexture(framebuffer_tex);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();
  return 0;
}