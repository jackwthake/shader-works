# Microcraft

## Hardware Requirements

### Adafruit PyGamer M4 Express
* **Board**: [Adafruit PyGamer M4 Express](https://www.adafruit.com/product/4242)
* **MCU**: SAMD51J19A ARM Cortex-M4F @ 200MHz with FPU
* **RAM**: 192KB SRAM
* **Flash**: 512KB onboard flash (8MB QSPI flash also available)
* **Display**: 160x128 ST7735R TFT LCD with 16-bit color (65K colors)
* **Input**:
  - 8 tactile buttons (D-pad, A, B, Start, Select)
  - Analog thumbstick (2-axis)
  - 3-axis accelerometer (LIS3DH)
* **Audio**: Built-in mono speaker and audio jack
* **Connectivity**: USB Type-C (for programming and power)
* **Power**:
  - USB powered or
  - JST connector for 3.7V LiPo battery
* **Features**:
  - NeoPixel LEDs (5x)
  - Light sensor
  - UF2 bootloader for drag-and-drop programming

### Desktop
* **OS**: Linux, Windows, or macOS
* **CPU**: Any modern x86-64 processor (multi-core recommended)
* **RAM**: 512MB minimum, 2GB+ recommended
* **GPU**: OpenGL-capable graphics card (for SDL3 rendering)
* **Display**: Any resolution
* **Build Tools**:
  - C++11 compatible compiler (GCC, Clang, or MSVC)
  - SDL3 library
  - CMake 3.15+

***

## Build Instructions

### Setup and Build

```bash
git clone --recursive https://github.com/jackwthake/shader-works.git
cd shader-works/demos/Microcraft

# Generate src/resources.inl (run any time files in res/ are updated)
python3 tools/bake.py

# Build with CMake (recommended)
mkdir build && cd build
cmake ..
cmake --build . -j4

# Upload to PyGamer M4
# Double-tap reset button to enter bootloader mode, then:
cmake --build . --target upload
```

## License

This project is open source and available under the MIT License.
