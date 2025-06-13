# Software Rasterizer for Adafruit PyGamer M4

This project is a software 3D rasterizer for the Adafruit PyGamer M4, written in C++ and designed to run on the Arduino framework. It demonstrates 3D rendering, double buffering, and QSPI flash filesystem usage on embedded hardware.

## Features
- 3D cube rendering with depth buffering
- Double buffering for smooth display
- QSPI flash filesystem support
- Serial debug console

## Hardware Requirements
- **Board:** Adafruit PyGamer M4
- **Display:** ST7735 (160x128)

## Setup Instructions

### 1. Install Arduino CLI
```sh
brew install arduino-cli  # macOS
# or download from https://arduino.github.io/arduino-cli/latest/installation/
```

### 2. Configure Arduino CLI
First, create a config file:
```sh
arduino-cli config init
```

Add Adafruit's package index URL to your Arduino CLI config:
```sh
arduino-cli config add board_manager.additional_urls https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
```

### 3. Install Required Components
Update the platform index:
```sh
arduino-cli core update-index
```

Install the Adafruit SAMD board platform:
```sh
arduino-cli core install adafruit:samd
```

Install all required libraries (as specified in platformio.ini):
```sh
arduino-cli lib install "Adafruit ST7735 and ST7789 Library@^1.5.5"
arduino-cli lib install "Adafruit GFX Library@^1.12.1"
arduino-cli lib install "Adafruit EPD@^4.5.6"
arduino-cli lib install "Adafruit SPIFlash@^5.1.1"
```

## Building and Uploading

A Python script (`build.py`) is provided to simplify the build and upload process.

### Basic Usage

1. **Compile Only**
   ```sh
   python3 build.py --compile
   ```
   The compiled binary will be stored in the `bin/` directory.

2. **Upload Only** (requires previous compilation)
   ```sh
   python3 build.py --upload --port <PORT>
   ```
   Replace `<PORT>` with your board's serial port (e.g., `/dev/ttyACM0` on Linux or `/dev/cu.usbmodemXXXX` on macOS)

3. **Compile and Upload**
   ```sh
   python3 build.py --all --port <PORT>
   ```

### Finding Your Board's Port

On macOS:
```sh
ls /dev/cu.usbmodem*
```

On Linux:
```sh
ls /dev/ttyACM*
```

### Build Script Options
```sh
python3 build.py --help
```

```
usage: build.py [-h] (--compile | --upload | --all) [--port PORT]

Build and upload Arduino project using arduino-cli.

optional arguments:
  -h, --help   show this help message and exit
  --compile    Only compile the project
  --upload     Only upload the project (requires prior compilation)
  --all        Compile and upload the project
  --port PORT  Serial port for upload (e.g. /dev/ttyACM0)
```

## Project Structure
- `Software_Rasterizer.ino` — Main Arduino sketch
- `src/` — Source files (renderer, model, helpers, etc.)
- `bin/` — Compiled binaries and intermediate files
- `build.py` — Python script for building and uploading
- `platformio.ini` — PlatformIO config (for reference)

## License
MIT

