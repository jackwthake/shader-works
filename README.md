<h1 align="center">Software Rasterizer</h1>
<p align="center">
  <br>
  <img src="https://github.com/user-attachments/assets/c049c8c7-0cde-44aa-a394-ef0f2d131587" alt="Demo project screen grab" />
  <img src="https://github.com/user-attachments/assets/282f91c4-77fa-4049-93c9-1343df47e52a" alt="" />
</p>

A **pure software 3D rasterizer** written in C11 that implements a complete graphics pipeline without GPU dependency. This project focuses on performance-oriented software rendering with a clean, modular API designed for learning and understanding 3D graphics fundamentals.

## Features

- **Complete Software Pipeline**: Full 3D rendering implementation from vertex transformation to pixel output
- **Function Pointer Shaders**: Extensible vertex and fragment shader system with rich context structures
- **Barycentric Rasterization**: Triangle rasterization using barycentric coordinates with depth testing
- **Transform-Based Camera**: Clean camera system using position + yaw/pitch rather than view matrices
- **Texture Atlas Support**: UV-based sampling from embedded texture atlas
- **Pixel Discard Transparency**: Fragment shaders can return `0x000000` to discard pixels for transparency effects
- **Built-in Primitives**: Geometry generators for cubes, spheres, and other common shapes
- **Diffuse Lighting**: Supports multiple directional and point lights per scene

## Quick Start
```c
#include <cpu-render/renderer.h>
#include <cpu-render/primitives.h>

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

```bash
# Clone with submodules
git clone --recursive https://github.com/jackwthake/Software-Rasterizer.git
cd Software-Rasterizer

# Or if already cloned, initialize submodules
git submodule update --init --recursive

# Configure build (Release recommended for performance)
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=RELEASE ..

# Build
cmake --build . -j 8

# Run demo
./basic_demo
```

### Build Configurations

- **Release**: `cmake -DCMAKE_BUILD_TYPE=RELEASE ..` (optimized for performance)
- **Debug**: `cmake -DCMAKE_BUILD_TYPE=DEBUG ..` (debugging symbols, no optimization)
- **RelWithDebInfo**: `cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..` (uses `-O3 -march=native -g`)

## Architecture

### Core Components
- **Renderer**: Complete 3D pipeline with vertex transformation, rasterization, and shading
- **Math Library**: Vector/matrix operations optimized for 3D graphics
- **Primitives**: Built-in generators for planes, cubes, and spheres
- **Shader System**: Extensible vertex and fragment shader architecture

### Technical Details
- **API**: Clean C11 interface with modular design (`cpu_render_lib`)
- **Dependencies**: SDL3 for platform abstraction (included as submodule)
- **Memory Model**: Client-provided framebuffer and depth buffer for flexibility
- **Threading**: Single-threaded design optimized for cache efficiency
- **Precision**: 32-bit floating point throughout the pipeline

