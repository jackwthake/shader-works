# Tundra

| | |
|:-------------------------:|:-------------------------:
| ![day-night cycle](./screenshots/day-night.gif) | ![snowy trees](./screenshots/snowy-trees.gif)
| ![static scene](./screenshots/static-1.png) |![static scene](./screenshots/static-2.png)

**An infinite explorable world rendered entirely in software.**

Built with the `shader-works` renderer to demonstrate real-world performance and capability. Features procedural terrain generation, real-time weather, and atmospheric effects — all running at 30-40 FPS on modest hardware.

## What It Demonstrates

**Complex Scene Management** — Handles 2000-3000 triangles per frame with chunk-based streaming and frustum culling.

**Dynamic Lighting** — Day/night cycle affects sun color, fog color, and ambient lighting in real-time.

**Environmental Effects** — Particle-based snow system with physics simulation and wind.

**Procedural Generation** — Infinite terrain using Perlin noise. Generates rolling hills, frozen lakes, gravel shores, and tree placement on the fly.

## Features

- Infinite procedural terrain generation
- 2-minute day/night cycle with atmospheric color transitions
- Real-time snow particle system
- Dynamic fog rendering
- Chunk-based world streaming
- First-person and overhead camera modes
- Wireframe debug view

## Controls

| Key | Action |
|-----|--------|
| W/A/S/D | Move |
| 1 | First-person view (default) |
| 2 | Birds-eye view |
| ESC | Exit |

## Building

```bash
# From shader-works root
./quick_build.sh release threads

# Run demo
./build/bin/tundra
```

Built with SDL3 for windowing and cJSON for configuration. Both included as submodules.
