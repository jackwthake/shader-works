<h1 align="center">Shader Works</h1>
<p align="center">
  <br>
  <img src="https://github.com/user-attachments/assets/c049c8c7-0cde-44aa-a394-ef0f2d131587" alt="Demo project screen grab" />
  <img src="https://github.com/user-attachments/assets/282f91c4-77fa-4049-93c9-1343df47e52a" alt="Demo prohect in wireframe mode" />
</p>

A **pure software 3D rasterizer** written in C11 that implements a complete graphics pipeline without GPU dependency. This project focuses on portability and performance-oriented software rendering with a clean, modular API designed for learning and understanding 3D graphics fundamentals.

## Features

- **Complete Software Pipeline**: Full 3D rendering implementation from vertex transformation to pixel output
- **As Portable As It Gets**: Just bring your own frame buffer, depth buffer, and color conversion functions
- **Function Pointer Shaders**: Extensible vertex and fragment shader system with rich context structures
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

## Quick Start
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

### Building

#### Quick Build (Recommended)
```bash
# Clone with submodules
git clone --recursive https://github.com/jackwthake/shader-works
cd shader-works

# Or if already cloned, initialize submodules
git submodule update --init --recursive

# Quick single configuration builds
./quick_build.sh release threads    # Multi-threaded release (best performance)
./quick_build.sh debug nothreads   # Single-threaded debug (easier debugging)
./quick_build.sh release          # Uses defaults: release with threading

# Build all configurations at once
./build_all.sh                    # Builds all 4 variants (debug/release × threaded/single)

# Run the demo
./build/bin/basic_demo                        # After quick_build.sh
# Or run specific configurations after build_all.sh:
# ./build-release-threaded/bin/basic_demo     # Release multi-threaded
# ./build-debug-single/bin/basic_demo         # Debug single-threaded
```

#### Manual CMake Build
```bash
# Configure with threading options
mkdir build && cd build

# Multi-threaded (default)
cmake -DCMAKE_BUILD_TYPE=Release -DSHADER_WORKS_USE_THREADS=ON ..

# Single-threaded (no pthread dependency)
cmake -DCMAKE_BUILD_TYPE=Debug -DSHADER_WORKS_USE_THREADS=OFF ..

# Build and run
cmake --build . -j 8
./bin/basic_demo
```

## Architecture

### Core Components
- **Renderer**: Complete 3D pipeline with vertex transformation, rasterization, and shading
- **Math Library**: Vector/matrix operations optimized for 3D graphics
- **Primitives**: Built-in generators for planes, cubes, and spheres
- **Shader System**: Extensible vertex and fragment shader architecture

### Technical Details
- **API**: Clean C11 interface with modular design (`cpu-render-lib`)
- **Dependencies**: Core library requires only libc and libm; optional pthread support; SDL3 for demo projects (included as submodule)
- **Memory Model**: Client-provided framebuffer and depth buffer for flexibility
- **Threading**: Optional POSIX threads implementation with triangle-level parallelization; single-threaded fallback optimized for cache efficiency
- **Precision**: 32-bit floating point with 8.24 fixed-point coordinates for subpixel accuracy
- **Safety**: Automatic bounds checking prevents buffer overruns and segfaults
- **Performance**: Optimized rasterization loop with perspective-correct interpolation and configurable multi-threading
