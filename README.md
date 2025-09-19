# Software Rasterizer
<p align="center">
  <img src="https://github.com/user-attachments/assets/d81ec894-9a02-4339-8f4b-fcfb6fc20eb3" alt="Demo project screen grab" />
</p>

A high-performance software 3D rendering engine written in C using SDL3. Implements a complete graphics pipeline entirely in software, featuring perspective projection, depth buffering, texture mapping, with programmable vertex and fragment shaders.

## Features

- **Pure Software Rendering**: Complete 3D pipeline without GPU dependency
- **Programmable Shaders**: Function pointer-based fragment shaders with custom parameters
- **Texture Mapping**: UV-based texture sampling from embedded atlas
- **Depth Buffering**: Z-buffer for proper 3D occlusion handling
- **Fixed Timestep**: 20 TPS game loop with frame interpolation for smooth rendering
- **Cross-Platform**: Built with SDL3 for broad compatibility

## Quick Start

### Building
```bash
# Clone with submodules
git clone --recursive https://github.com/jackwthake/Software-Rasterizer.git
cd Software-Rasterizer

# Builds
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j 8

# Run demo
./basic_demo
```

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

