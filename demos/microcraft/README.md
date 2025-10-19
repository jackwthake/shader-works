# Microcraft

<p align="center">
  <img src="./screenshots/screenshot.png" width="400" alt="3D rendering output"/>
  <img src="./screenshots/embedded.jpeg" width="400" alt="Running on hardware"/>
</p>

**The same software renderer from the main project, running on a microcontroller.**

This demo proves the portability claim — the exact same rendering code that runs on desktop also runs on a 200MHz ARM Cortex-M4 with just 192KB of RAM.

## What Makes This Interesting

**No GPU Required** — 3D rendering with texturing, depth buffering, and perspective correction on bare metal hardware with no graphics acceleration.

**Real Hardware Constraints** — 160x128 pixel display, 192KB RAM, 200MHz CPU. Achieves playable framerates through optimized rasterization and careful memory management.

**Identical Codebase** — Uses the same `shader-works` library as desktop demos. Only difference is the platform layer (display driver, input handling).

## Hardware Target

**Adafruit PyGamer M4 Express** ([Product Page](https://www.adafruit.com/product/4242))
- SAMD51J19A ARM Cortex-M4F @ 200MHz with hardware FPU
- 192KB SRAM, 512KB Flash
- 160x128 ST7735R TFT display (65K colors)
- Joystick + buttons for input

## Building and Uploading

```bash
# From shader-works root
cd demos/microcraft

# Embed texture assets
python3 tools/bake.py

# Build firmware
mkdir build && cd build
cmake .. && cmake --build . -j4

# Upload to device
# 1. Double-tap reset button on PyGamer (should see "PYGAMER" drive)
# 2. Run upload target
cmake --build . --target upload
```

The device should reboot and start rendering immediately.
