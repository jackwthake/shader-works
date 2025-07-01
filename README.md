# Software Rasterizer for Adafruit PyGamer M4

A real-time 3D software rasterizer written in C++ for the Adafruit PyGamer M4 development board. This project demonstrates advanced embedded graphics programming, featuring custom 3D rendering, texture mapping, and QSPI flash-based resource management on ARM Cortex-M4 hardware.

## Features

- **Real-time 3D Rendering**: Custom software rasterizer with perspective projection
- **Depth Buffering**: Z-buffer implementation for proper 3D object occlusion
- **Double Buffering**: Smooth 160x128 display updates using framebuffer swapping
- **Texture Mapping**: BMP texture loading and RGB565 format conversion
- **QSPI Flash Storage**: Efficient resource management using onboard flash memory
- **Model Loading**: OBJ file parser for 3D geometry
- **Performance Optimized**: Hand-tuned for 200MHz SAMD51 processor
- **Serial Debug Console**: Real-time debugging and performance monitoring

## Hardware Requirements

- **Board**: Adafruit PyGamer M4 Express
- **MCU**: SAMD51J19A ARM Cortex-M4F @ 200MHz (overclocked from 120MHz stock)
- **RAM**: 192KB SRAM
- **Flash**: 512KB onboard + 8MB QSPI flash storage  
- **Display**: 160x128 ST7735 TFT with 16-bit color
- **Input**: action buttons, and analog joystick

---

## Quick Start

### Prerequisites

- Python 3.7+
- Git with submodule support
- Make sure you have ~2GB free space for toolchain and dependencies

### Setup and Build

1. **Clone the Repository**
   ```bash
   git clone --recursive https://github.com/your-username/software-rasterizer.git
   cd software-rasterizer
   ```

2. **Initialize Submodules** (if not cloned with `--recursive`)
   ```bash
   git submodule update --init --recursive
   ```

3. **Download ARM Toolchain**
   
   The project uses a self-contained ARM GCC toolchain. Download and extract it to the `tools/` directory:
   ```bash
   # Download ARM GCC 9-2019-q4-major for your platform
   # For macOS/Linux:
   cd tools
   wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2019q4/gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2
   tar -xjf gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2
   mv gcc-arm-none-eabi-9-2019-q4-major arm-none-eabi-gcc/9-2019q4
   cd ..
   ```

4. **⚠️ IMPORTANT: Flash Format Setup (Required on First Install)**
   
   Before building the main project, you **must** format the QSPI flash and upload resources:
   
   ```bash
   # Navigate to flash format utility
   cd tools/flash_format
   
   # Build the flash formatter
   chmod +x build
   ./build
   
   # Put PyGamer in bootloader mode (double-click reset button)
   # Flash the formatter binary using bossac
   ../bossac -i -d --port=<PORT> -U -i --offset=0x4000 -w -v "bin/flash_format.bin" -R
   
   # Connect to serial monitor at 115200 baud to complete formatting
   # Return to main directory
   cd ../..
   ```
   
   **Note**: This step is required on first installation or whenever files in the `res/` directory are modified.

5. **Build the Main Project**
   ```bash
   chmod +x build
   ./build
   ```

6. **Flash Main Program to Device**
   ```bash
   # Put PyGamer in bootloader mode (double-click reset button)
   # Flash the binary using bossac
   ./tools/bossac -i -d --port=<PORT> -U -i --offset=0x4000 -w -v "bin/software-rasterizer.bin" -R
   ```

## Project Architecture

The project is organized as follows:

```
software-rasterizer/
├── src/                        # Main source code
│   ├── device.cpp/h            # Hardware abstraction layer
│   ├── renderer.cpp/h          # 3D rendering engine
│   ├── resource_manager.cpp/h  # Asset loading and management
│   └── util/                   # Math and utility functions
├── lib/                        # External dependencies (git submodules)
│   ├── CMSIS/                  # ARM CMSIS libraries
│   ├── CMSIS-Atmel/            # ARM CMSIS-Atmel
│   ├── samd-core/              # Adafruit SAMD core
│   ├── SPIFlash/               # SPI flash library
│   └── SdFat/                  # FAT filesystem
├── tools/                      # Build tools and ARM toolchain
├── res/                        # Resources (textures, models)
├── bin/                        # Compiled binaries and object files
└── build                       # Python build script
```

## Build System

This project uses a custom Python-based build system that:

- **Cross-compiles** C/C++ source files using ARM GCC
- **Manages dependencies** through git submodules
- **Links** object files into the final ELF/binary
- **Optimizes** for size and performance on SAMD51

### Build Script Features

- Incremental compilation (only rebuilds changed files)
- Automatic dependency resolution
- Warning collection and reporting
- Cross-platform support (macOS, Linux, Windows)

### Manual Build Steps

If you prefer to understand the build process:

1. **Compile Core Libraries**
   ```bash
   # The build script compiles:
   # - SAMD core (Arduino framework)
   # - SPI/Wire libraries  
   # - USB stack
   # - Zero DMA library
   ```

2. **Compile Dependencies**
   ```bash
   # External libraries:
   # - SdFat filesystem
   # - SPIFlash driver
   ```

3. **Compile Project Sources**
   ```bash
   # source files in src/
   ```

4. **Link Everything**
   ```bash
   # Creates final ELF and binary files
   ```

---

## Resource Management

### ⚠️ Critical First Step: QSPI Flash Setup

**This step is mandatory before the main rasterizer will function properly.**

Before running the main rasterizer, you **must** format the QSPI flash and upload resources using the flash format utility. This is required:
- On first installation
- Whenever files in the `res/` directory are modified
- If the QSPI flash becomes corrupted

### Flash Format Process

1. **Navigate to flash format utility**
   ```bash
   cd tools/flash_format
   ```

2. **Generate resource files**
   ```bash
   python3 res_copy.py
   ```
   This creates the necessary `files.h` header with embedded resources.

3. **Build and flash the formatter**
   ```bash
   # Build the flash formatter
   chmod +x build
   ./build
   
   # Put PyGamer in bootloader mode (double-click reset button)
   # Flash the formatter binary using bossac
   ../bossac -i -d --port=/dev/cu.usbmodem141401 -U -i --offset=0x4000 -w -v "bin/flash_format.bin"
   ```

4. **Complete the formatting process**
   - Connect to the board's serial port at **115200 baud**
   - The formatter will wait for a serial connection before proceeding
   - Monitor the output to ensure successful formatting
   - Return to the main directory: `cd ../..`

**⚠️ Warning**: The main software rasterizer will not work correctly without this step. If you see resource loading errors or blank screens, ensure the flash format process was completed successfully.

### Resource Files

- **atlas.bmp**: Texture atlas for 3D models
- **cube.obj**: 3D model data in Wavefront OBJ format
- **test.txt**: Sample text data

---

## Development

### Debug Console

Connect to the PyGamer's serial port at **115200 baud** for:
- Real-time performance metrics
- Memory usage statistics  
- Error messages and warnings
- Test results output

### Performance Optimization

The rasterizer is optimized for the SAMD51's constraints:

- **Memory**: Careful buffer management within 192KB RAM
- **Speed**: Integer math where possible, lookup tables
- **Power**: Efficient rendering loops, minimal allocations

---

## Troubleshooting

### Common Issues

1. **Build fails with "Compiler not found"**
   - Ensure ARM GCC toolchain is properly extracted to `tools/arm-none-eabi-gcc/9-2019q4/`
   - Check file permissions on the compiler binaries

2. **Submodule errors**
   ```bash
   git submodule update --init --recursive --force
   ```

3. **Flash not detected**
   - Double-click reset button to enter bootloader mode
   - Check USB connection and port permissions
   - Try a different USB cable

4. **Display issues**
   - **First check**: Verify QSPI flash is properly formatted using the flash format utility in `tools/flash_format`
   - Check resource files are uploaded correctly
   - Monitor serial output for error messages
   - If you see "resource not found" errors, re-run the flash format process

### Getting Help

- Check the serial console output first
- Review compiler warnings in build output
- Ensure all dependencies are properly initialized

---

## License

This project is open source and available under the MIT License.
You are free to use, modify, and distribute this code as long as you include the original license and copyright notice.

---

*Built with ❤️ for embedded graphics programming*