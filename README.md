<h1 align="center">Shader Works</h1>
<p align="center">
  <br>
  <img src="https://github.com/user-attachments/assets/c049c8c7-0cde-44aa-a394-ef0f2d131587" alt="Demo project screen grab" />
  <img src="https://github.com/user-attachments/assets/282f91c4-77fa-4049-93c9-1343df47e52a" alt="Demo prohect in wireframe mode" />
</p>

<p align="center">
  <a href="https://github.com/jackwthake/shader-works/actions/workflows/quick-build.yml">
    <img src="https://github.com/jackwthake/shader-works/actions/workflows/quick-build.yml/badge.svg" alt="Mac Linux">
  </a>
  <a href="https://github.com/jackwthake/shader-works/actions/workflows/windows-build.yml">
    <img src="https://github.com/jackwthake/shader-works/actions/workflows/windows-build.yml/badge.svg" alt="Windows">
  </a>
</p>

A **pure software 3D rasterizer** written in C11 that implements a complete graphics pipeline without GPU dependency. This project focuses on portability and performance-oriented software rendering with a clean, modular API designed for learning and understanding 3D graphics fundamentals.

## Table of Contents
- [Features](#features)
- [Quick Start](#quick-start)
- [Building](#building)
- [Cross-Platform Builds](#cross-platform-builds)
- [Architecture](#architecture)
- [API Reference](#api-reference)

# Features

- **Complete Software Pipeline**: Full 3D rendering implementation from vertex transformation to pixel output
- **As Portable As It Gets**: Just bring your own frame buffer, depth buffer, and color conversion functions
- **Programmable Shaders**: Function pointer-based vertex and fragment shaders with rich context structures and user-defined parameters
- **Barycentric Rasterization**: Triangle rasterization using barycentric coordinates with depth testing
- **Transform-Based Camera**: Clean camera system using position + yaw/pitch rather than view matrices
- **Texture Atlas Support**: UV-based sampling from embedded texture atlas
- **Pixel Discard Transparency**: Fragment shaders can return `rgb_to_u32(255, 0, 255)` to discard pixels for transparency effects
- **Wireframe Rendering**: Built-in wireframe mode with edge detection using barycentric coordinates
- **Perspective-Correct Texturing**: Proper UV interpolation with 1/w correction for realistic texture mapping
- **Built-in Primitives**: Geometry generators for cubes, spheres, and other common shapes
- **Diffuse Lighting**: Supports multiple directional and point lights per scene
- **Multi-threaded Rendering**: Optional POSIX threads support for parallel triangle rasterization
- **Configurable Threading**: Build with or without pthread dependency via CMake options

# Quick Start
```c
#include <shader-works/renderer.h>
#include <shader-works/primitives.h>

// Required: implement color conversion for your target pixel format
uint32_t rgb_to_u32(uint8_t r, uint8_t g, uint8_t b) {
  return (r << 24) | (g << 16) | (b << 8) | 0xFF; // RGBA8888 example
}

void u32_to_rgb(uint32_t color, uint8_t *r, uint8_t *g, uint8_t *b) {
  *r = (color >> 24) & 0xFF; *g = (color >> 16) & 0xFF; *b = (color >> 8) & 0xFF;
}

int main() {
  // Setup window and framebuffer...

  uint32_t framebuffer[WIDTH * HEIGHT];
  float depthbuffer[WIDTH * HEIGHT];
  renderer_t renderer_state = {0};

  init_renderer(&renderer_state, WIDTH, HEIGHT, 0, 0,
                framebuffer, depthbuffer, MAX_DEPTH);

  transform_t camera = {0};
  update_camera(&renderer_state, &camera);

  model_t cube = {0};
  generate_cube(&cube, make_float3(0, 0, -5), make_float3(1, 1, 1));

  while (running) {
    // Clear buffers
    for(int i = 0; i < WIDTH * HEIGHT; ++i) {
      framebuffer[i] = 0x000000;
      depthbuffer[i] = FLT_MAX;
    }

    render_model(&renderer_state, &camera, &cube, NULL, 0);

    // Present framebuffer to screen...
  }

  delete_model(&cube);
  return 0;
}
```

# Building

## Quick Build (Recommended)
```bash
# Clone with submodules
git clone --recursive https://github.com/jackwthake/shader-works
cd shader-works

# Or if already cloned, initialize submodules
git submodule update --init --recursive

# Quick single configuration builds
./quick_build.sh release threads           # Multi-threaded release (best performance)
./quick_build.sh debug nothreads           # Single-threaded debug (easier debugging)
./quick_build.sh release                   # Uses defaults: release with threading

# Build all configurations at once
./build_all.sh                             # Builds all 4 variants (debug/release × threaded/single)

# Run the demo
./build/bin/basic_demo                     # After quick_build.sh
# Or run specific configurations after build_all.sh:
# ./build-release-threaded/bin/basic_demo  # Release multi-threaded
# ./build-debug-single/bin/basic_demo      # Debug single-threaded
```

## Manual CMake Build
```bash
# Configure with threading options
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DSHADER_WORKS_USE_THREADS=ON ..
cmake --build . -j 8 && ./bin/basic_demo
```

# Cross-Platform Builds

## Windows (MinGW-w64)
```bash
# Install via MSYS2
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake

./quick_build.sh release threads
./build/bin/basic_demo.exe
```

## macOS
```bash
brew install cmake  # Xcode Command Line Tools assumed

./quick_build.sh release threads
```

## Other Linux Distros
```bash
# Ubuntu/Debian: sudo apt install build-essential cmake
# Fedora/RHEL: sudo dnf install gcc cmake
# Arch Linux: sudo pacman -S base-devel cmake

./quick_build.sh release threads
```

# Architecture

## Core Components
- **Renderer**: Complete 3D pipeline with vertex transformation, rasterization, and shading
- **Math Library**: Vector/matrix operations optimized for 3D graphics
- **Primitives**: Built-in generators for planes, cubes, and spheres
- **Shader System**: Programmable vertex and fragment shaders with context-rich function pointers

## Technical Details
- **API**: Clean C11 interface with modular design (`cpu-render-lib`)
- **Dependencies**: Core library requires only libc and libm; optional pthread support; SDL3 for demo projects (included as submodule)
- **Memory Model**: Client-provided framebuffer and depth buffer for flexibility
- **Threading**: Optional POSIX threads implementation with triangle-level parallelization; single-threaded fallback optimized for cache efficiency
- **Precision**: 32-bit floating point with 8.24 fixed-point coordinates for subpixel accuracy
- **Safety**: Automatic bounds checking prevents buffer overruns and segfaults
- **Performance**: Optimized rasterization loop with perspective-correct interpolation and configurable multi-threading

# API Reference

## renderer.h
### Required Client Functions

```c
u32 rgb_to_u32(u8 r, u8 g, u8 b);
void u32_to_rgb(u32 color, u8 *r, u8 *g, u8 *b);
```
Client must implement these for pixel format conversion. Allows renderer to be pixel-format agnostic.

### Core Renderer Functions
```c
void init_renderer(renderer_t *state, u32 win_width, u32 win_height,
                   u32 atlas_width, u32 atlas_height, u32 *framebuffer,
                   f32 *depthbuffer, f32 max_depth);
```
Initialize renderer with client-provided buffers. Framebuffer and depthbuffer must be pre-allocated arrays of `win_width * win_height` elements.

---
```c
void update_camera(renderer_t *state, transform_t *cam);
```
Update camera basis vectors based on transform (position + yaw/pitch). Call before rendering.

---
```c
usize render_model(renderer_t *state, transform_t *cam, model_t *model,
                   light_t *lights, usize light_count);
```
Render model with threading support. Returns number of triangles rendered. Handles vertex transformation, rasterization, and shading.

---
```c
void apply_fog_to_screen(renderer_t *state, f32 fog_start, f32 fog_end,
                         u8 fog_r, u8 fog_g, u8 fog_b);
```
Apply depth-based fog effect to entire framebuffer. Fog interpolates between `fog_start` and `fog_end` distances.

---
## primitives.h

### Data Structures
Use the `model_t` structure to store model data for use with the renderer. Vertices must be in Counter-Clockwise Winding (CCW) order for proper back-face culling.
```c
typedef struct {
  vertex_data_t *vertex_data;     // Position, UV, normal per vertex
  float3 *face_normals;           // Per-triangle normals for back-face culling
  usize num_vertices, num_faces;

  transform_t transform;          // Position, yaw, pitch
  bool use_textures;              // If false, use flat shading

  vertex_shader_t *vertex_shader;
  fragment_shader_t *frag_shader;
} model_t;
```


### Geometry Generation
```c
// Generator functions
int generate_cube(model_t* model, float3 position, float3 size);
int generate_sphere(model_t* model, f32 radius, int segments, int rings, float3 position);
int generate_plane(model_t* model, float2 size, float2 segment_size, float3 position);
int generate_quad(model_t* model, float2 size, float3 position);
void delete_model(model_t* model);
```

All generators allocate and populate model with vertices, normals, and UV coordinates. Return 0 on success. **cube**: axis-aligned box. **sphere**: UV sphere with configurable tessellation. **plane**: subdivided for displacement effects. **quad**: simple 2-triangle surface. Always call `delete_model()` to free memory.

## shaders.h

### Shader Creation

```c
vertex_shader_t make_vertex_shader(vertex_shader_func func, void *argv, usize argc);
fragment_shader_t make_fragment_shader(fragment_shader_func func, void *argv, usize argc);
```
Create custom shaders with user-defined arguments. Arguments are passed to shader function on every invocation.

### Shader Function Signatures

```c
typedef float3 (*vertex_shader_func)(vertex_context_t *context, void *args, usize argc);
typedef u32 (*fragment_shader_func)(u32 input_color, fragment_context_t *context, void *args, usize argc);
```

**Vertex shaders** transform vertices from model space and return modified position. Context provides camera data, original vertex info, and timing.

**Fragment shaders** process pixels and return final color. Return `rgb_to_u32(255, 0, 255)` to discard pixel for transparency.

### Built-in Shaders

```c
extern vertex_shader_t default_vertex_shader;           // Standard MVP transformation
extern fragment_shader_t default_frag_shader;           // Textured
extern fragment_shader_t default_lighting_frag_shader;  // Multi-light support
```

### Context Structures
The shader system provides extensive programmability through rich context structures and user-defined parameters. Context structures expose render pipeline data while custom parameter buffers enable dynamic effects with game data.
**fragment_context_t** provides:
- `world_pos`, `screen_pos`, `uv`, `depth` - spatial information
- `normal`, `view_dir` - lighting vectors
- `time` - for animations
- `light`, `light_count` - scene lighting

**vertex_context_t** provides:
- Camera vectors (`cam_position`, `cam_forward`, `cam_right`, `cam_up`)
- Projection parameters (`projection_scale`, `frustum_bound`, `screen_dim`)
- Original vertex data (`original_vertex`, `original_uv`, `original_normal`)
- Indices (`vertex_index`, `triangle_index`) and timing (`time`)

### Lighting
Can be used within fragment shaders to add lighting effects. Use `default_lighting_frag_shader` for basic diffuse lighting with no shadows.
```c
typedef struct {
  float3 position, direction;
  u32 color;
  bool is_directional;  // true = directional light, false = point light
} light_t;
```
