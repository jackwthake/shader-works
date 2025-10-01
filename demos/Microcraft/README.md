# Minicraft for Adafruit Pygamer M4 Express

This project is a real-time 3D Minecraft clone written in C++ for the Adafruit PyGamer M4. It showcases a 3D software rasterizer with texture mapping and resource management on an ARM Cortex-M4 microcontroller.

***

## Features

* **Real-time 3D Rendering**: Custom software rasterizer with perspective projection.
* **Depth Buffering**: Z-buffer implementation for correct 3D object occlusion.
* **Texture Mapping**: Texture-mapped blocks using a shared texture atlas.
* **Frustum Culling**: Doesn't attempt to render blocks outside the viewport
* **Occlusion Culling**: Doesn't attempt to render blocks hidden from view
* **Resource Baking**: A Python script bakes resources directly into the source code, eliminating the need for a separate flash storage utility.
* **Performance Optimized**: Tuned for the 200MHz SAMD51 processor, Hot functions make use of caching as many constants as possible.
* **Serial Debug Console**: Real-time debugging and performance monitoring.

***

## Hardware Requirements

* **Board**: Adafruit PyGamer M4 Express
* **MCU**: SAMD51J19A ARM Cortex-M4F @ 200MHz
* **RAM**: 192KB SRAM
* **Flash**: 512KB onboard flash
* **Display**: 160x128 ST7735 TFT with 16-bit color
* **Input**: Onboard buttons and analog joystick

***

## Build Instructions

### Prerequisites

* Python 3.7+
* Git with submodule support
* ARM GCC toolchain (e.g., `arm-none-eabi-gcc`)

### Setup and Build
    git clone --recursive https://github.com/jackwthake/software-rasterizer.git
    cd software-rasterizer

    # generate src/resources.inl, this must be ran any time files in res/ are updated
    python3 tools/bake.py

    # Build src
    chmod +x build
    ./build
    
    # Upload
    bossac -i -d --port=<YOUR_PORT> -U -i --offset=0x4000 -w -v "bin/software-rasterizer.bin" -R
***

## Project Structure

The project is organized as follows:
```
├── src/                # Main source code
│   ├── main.cpp        # Main application entry point
│   ├── scene.cpp/h     # Scene management and game logic
│   ├── renderer.cpp/h  # 3D rendering engine
│   ├── device.h        # Hardware abstraction layer
│   └── input.cpp       # Input code
│   └── util/           # Math and utility functions
├── lib/                # External dependencies (submodules)
├── tools/              # Build tools and scripts
│   └── bake.py         # Resource baking script
├── res/                # Resource files (textures)
├── bin/                # Directory for final binary / intermediate files
└── build               # Build script
```

***

## Development

Connect to the PyGamer's serial port at **115200 baud** to view real-time debug information, performance metrics, and memory usage statistics.

***

## License

This project is open source and available under the MIT License.
