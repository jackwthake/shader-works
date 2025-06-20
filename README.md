# Software Rasterizer for Adafruit PyGamer M4

This project is a software 3D rasterizer for the Adafruit PyGamer M4, written in C++ and designed to run on the Arduino framework. It demonstrates 3D rendering, double buffering, and QSPI flash filesystem usage on embedded hardware.

## Features
- 3D cube rendering with depth buffering
- Double buffering for smooth display
- QSPI flash filesystem support
- Serial debug console

## Hardware Specs
- **Board:** Adafruit PyGamer M4
  - **CPU** SAMD51J19 at Running at 200MHZ (120MHZ Stock)
    - 192kb RAM
    - 512kb Onboard QSPI flash
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
arduino-cli lib install "Adafruit ST7735 and ST7789 Library"
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "Adafruit SPIFlash"
```

## Building and Uploading
A Python script (`build.py`) is provided to simplify the build and upload process of both the main ```Software_Rasterizer.ino``` Sketch and the ```flash_format.ino``` sketch.

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

### Flash Format and Resource Generation
In order for the main sketch to load the necessary resources, you must compile and upload the ```flash_format.ino``` sketch found in ```flash_format/``` before uploading the main sketch.

1. **Generate Resource Files**
  ```sh
  cd flash_format
  python3 res_copy.py
  ```
  This creates the necessary `files.h` header with embedded resources.

2. **Compile and Upload Flash Format**
  ```sh
  ./build.py --all --port <PORT>
  ```
  Once uploaded the board will wait for a serial connection with a baud rate of ```115200``` before writing the necessary files to the onboard flash.

  NOTE: This sketch only needs to be uploaded uppon first installation, or uppon any modification to the files in the ```res/``` directory.

### Compiling and Uploading Main Sketch (Software_Rasterizer.ino)

After formatting the flash and generating resources, you can build and upload the main sketch:

```sh
cd ../
./build.py --all --port <PORT>
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

