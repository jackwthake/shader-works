# Software Rasterizer for Adafruit PyGamer M4

This project is a 3D rasterizer for the Adafruit PyGamer M4, written in C++ and designed to run on the Arduino framework. It demonstrates 3D rendering, double buffering, and QSPI flash filesystem usage on embedded hardware.

## Features

- 3D cube rendering with depth buffering
- Double buffering for smooth display
- QSPI flash filesystem support
- Serial debug console
- Basic unit testing suite

## Hardware Specs

- **Board:** Adafruit PyGamer M4
- **CPU** SAMD51J19 overclocked to 200Mhz (120mhz Stock)
- **RAM** 192Kb
- **STORAGE** 512Kb Onboard QSPI flash
- **Display:** ST7735 (160x128)

---

## Arduino-cli and Dependency Configuration

To set up the project, you'll need the Arduino CLI and several libraries. Here's how to configure everything:

1.  **Install Arduino CLI**

    Follow the official installation guide for Arduino CLI: [Arduino CLI Installation Guide](https://arduino.github.io/arduino-cli/latest/installation/)

2.  **Configure Arduino CLI**

    First, create a config file:

    ```sh
    arduino-cli config init
    ```

    Add Adafruit's package index URL to your Arduino CLI config:

    ```sh
    arduino-cli config add board_manager.additional_urls [https://adafruit.github.io/arduino-board-index/package_adafruit_index.json](https://adafruit.github.io/arduino-board-index/package_adafruit_index.json)
    ```

3.  **Install Board and Libraries**

    ```sh
    # update libraries
    arduino-cli core update-index
    # Install the Adafruit SAMD board platform:
    arduino-cli core install adafruit:samd --additional-urls "[https://adafruit.github.io/arduino-board-index/package_adafruit_index.json](https://adafruit.github.io/arduino-board-index/package_adafruit_index.json)"
    # Install required libraries:
    arduino-cli lib install "Adafruit ST7735 and ST7789 Library"
    arduino-cli lib install "Adafruit GFX Library"
    arduino-cli lib install "Adafruit SPIFlash"
    ```

---

## Usage of the `build.py` script

A Python script (`build.py`) is provided to simplify the build and upload process of both the main `Software_Rasterizer.ino` Sketch and the `flash_format.ino` sketch.

### Basic Usage

1.  **Compile Only**

    ```sh
    python3 build.py --compile
    ```

    The compiled binary will be stored in the `bin/` directory.

2.  **Upload Only** (requires previous compilation)

    ```sh
    python3 build.py --upload --port <PORT> --monitor
    ```

    Replace `<PORT>` with your board's serial port (e.g., `/dev/ttyACM0` on Linux or `/dev/cu.usbmodemXXXX` on macOS)
    
    ```--monitor```: Upon upload success, open a serial monitor with the same port (Optional)

3.  **Compile and Upload**

    ```sh
    python3 build.py --all --port <PORT> --monitor
    ```

### Flash Format and Resource Generation

In order for the main sketch to load the necessary resources, you must compile and upload the `flash_format.ino` sketch found in `flash_format/` before uploading the main sketch.

1.  **Generate Resource Files**

    ```sh
    cd flash_format
    python3 res_copy.py
    ```

    This creates the necessary `files.h` header with embedded resources.

2.  **Compile and Upload Flash Format**

    ```sh
    ./build.py --all --port <PORT> --monitor
    ```

    Once uploaded the board will wait for a serial connection with a baud rate of `115200` before writing the necessary files to the onboard flash.

    NOTE: This sketch only needs to be uploaded upon first installation, or upon any modification to the files in the `res/` directory.

### Compiling and Uploading Main Sketch (Software_Rasterizer.ino)

After formatting the flash and generating resources, you can build and upload the main sketch:

```sh
cd ../
./build.py --all --port <PORT>
```

## Test Files and Unit Testing

The project includes a set of unit tests located in the `tests/` directory to verify the functionality of various components.

The main `Software_Rasterizer.ino` sketch can be switched into a dedicated unit test mode by defining the `RUN_BOARD_TESTS` macro. When this macro is defined, instead of running the graphical rasterizer, the sketch will execute the tests and print their results to the serial console.

### Test Files Overview

-   `test.cpp`: Contains the main entry point for the test runner and orchestrates the execution of other tests.
-   `test.h`: Exposes the entry point to the test runner
-   `expected_values.h`: Holds expected data for operations, some info is generated ```utils/rgb_convert.py```

### How to Run Tests

```c++
#define RUN_BOARD_TESTS // must define before include for tests to compile and run
#include "test.h"
```

Once uploaded, connect to the board's serial port (e.g., using a serial monitor at 115200 baud) to view the test results.