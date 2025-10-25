#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <time.h>

#include <shader-works/renderer.h>
#include <shader-works/maths.h>
#include <shader-works/primitives.h>

#define WIN_WIDTH 800
#define WIN_HEIGHT 600
#define MAX_DEPTH 100

// Simple color conversion (no SDL needed for benchmarking)
u32 rgb_to_u32(u8 r, u8 g, u8 b) {
  return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}

void u32_to_rgb(u32 color, u8 *r, u8 *g, u8 *b) {
  *r = (color >> 24) & 0xFF;
  *g = (color >> 16) & 0xFF;
  *b = (color >> 8) & 0xFF;
}

// Get current time in seconds
static double get_time_seconds() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Run benchmark with given scene complexity
static void benchmark_scene(FILE *csv, int sphere_subdivisions, int num_frames) {
  // Allocate buffers
  u32 *framebuffer = malloc(WIN_WIDTH * WIN_HEIGHT * sizeof(u32));
  f32 *depthbuffer = malloc(WIN_WIDTH * WIN_HEIGHT * sizeof(f32));

  renderer_t renderer_state = {0};
  init_renderer(&renderer_state, WIN_WIDTH, WIN_HEIGHT, 0, 0, framebuffer, depthbuffer, MAX_DEPTH);

  // Create test scene - sphere with varying complexity
  model_t sphere = {0};
  generate_sphere(&sphere, 2.0f, sphere_subdivisions, sphere_subdivisions, make_float3(0.0f, 0.0f, -6.0f));
  sphere.frag_shader = &default_lighting_frag_shader;
  sphere.vertex_shader = &default_vertex_shader;
  sphere.use_textures = false;

  // Setup camera and lighting
  transform_t camera = {0};
  camera.position = make_float3(0.0f, 0.0f, 0.0f);
  update_camera(&renderer_state, &camera);

  light_t sun = {
    .is_directional = true,
    .direction = make_float3(-1, -1, -1),
    .color = rgb_to_u32(255, 255, 255)
  };

  // Warm-up pass
  for(int i = 0; i < 5; ++i) {
    for(int j = 0; j < WIN_WIDTH * WIN_HEIGHT; ++j) {
      framebuffer[j] = rgb_to_u32(0, 0, 0);
      depthbuffer[j] = FLT_MAX;
    }
    render_model(&renderer_state, &camera, &sphere, &sun, 1);
  }

  // Benchmark
  double start_time = get_time_seconds();
  usize total_triangles = 0;

  for(int frame = 0; frame < num_frames; ++frame) {
    // Clear buffers
    for(int i = 0; i < WIN_WIDTH * WIN_HEIGHT; ++i) {
      framebuffer[i] = rgb_to_u32(0, 0, 0);
      depthbuffer[i] = FLT_MAX;
    }

    // Animate sphere
    sphere.transform.yaw += 0.01f;
    sphere.transform.pitch += 0.005f;

    // Render and count triangles
    total_triangles += render_model(&renderer_state, &camera, &sphere, &sun, 1);
  }

  double end_time = get_time_seconds();
  double elapsed = end_time - start_time;
  double fps = num_frames / elapsed;
  double triangles_per_sec = total_triangles / elapsed;

  // Output to CSV
  fprintf(csv, "%d,%zu,%.2f,%.2f,%.0f\n",
          sphere_subdivisions,
          sphere.num_faces,
          elapsed,
          fps,
          triangles_per_sec);

  // Cleanup
  delete_model(&sphere);
  free(framebuffer);
  free(depthbuffer);
}

int main(int argc, char *argv[]) {
  printf("Shader-Works Renderer Benchmark\n");
  printf("================================\n\n");

#ifdef SHADER_WORKS_USE_PTHREADS
  printf("Threading: ENABLED\n");
#else
  printf("Threading: DISABLED\n");
#endif

  printf("Resolution: %dx%d\n", WIN_WIDTH, WIN_HEIGHT);
  printf("Frames per test: 100\n\n");

  // Open CSV file for output
  const char *filename = "benchmark_results.csv";
  FILE *csv = fopen(filename, "w");
  if (!csv) {
    fprintf(stderr, "Failed to open %s for writing\n", filename);
    return 1;
  }

  // CSV header
  fprintf(csv, "subdivisions,triangles,time_sec,fps,tri_per_sec\n");

  // Test with increasing complexity
  int test_cases[] = {4, 8, 16, 24, 32, 48, 64};
  int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);

  for(int i = 0; i < num_tests; ++i) {
    int subdivisions = test_cases[i];
    printf("Testing with %d subdivisions... ", subdivisions);
    fflush(stdout);

    benchmark_scene(csv, subdivisions, 100);

    printf("done\n");
  }

  fclose(csv);

  printf("\nResults written to %s\n", filename);
  printf("Columns: subdivisions, triangles, time_sec, fps, tri_per_sec\n");

  return 0;
}
