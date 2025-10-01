cmake_minimum_required(VERSION 3.15)

# Set the toolchain file before project()
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/arm-none-eabi-toolchain.cmake)

project(microcraft C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project configuration
set(PROG_NAME "microcraft")
set(MCU_PART "SAMD51J19A")

# Directories
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/platforms/samd51)
set(LINKER_SCRIPT ${PLATFORM_DIR}/variant/linker_scripts/gcc/flash_with_bootloader.ld)

# Set unified output directory in build/bin
set(UNIFIED_BIN_DIR ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${UNIFIED_BIN_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Compiler definitions
add_compile_definitions(
    F_CPU=200000000L
    ARDUINO=10607
    ARDUINO_PYGAMER_M4
    ARDUINO_ARCH_SAMD
    ARDUINO_SAMD_ADAFRUIT
    __SAMD51J19A__
    CRYSTALLESS
    ADAFRUIT_PYGAMER_M4_EXPRESS
    __SAMD51__
    __FPU_PRESENT
    ARM_MATH_CM4
    USB_VID=0x239A
    USB_PID=0x803D
    USBCON
    USB_CONFIG_POWER=100
    USB_MANUFACTURER="Adafruit"
    USB_PRODUCT="PyGamer M4 Express"
    __SKETCH_NAME__="${PROG_NAME}"
)

# Common compiler flags for ARM Cortex-M4
set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
)

# C flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3 -Wall -std=gnu11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto -ffast-math -funroll-loops")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib")

# C++ flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -Wall -std=gnu++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -ffast-math -funroll-loops")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-threadsafe-statics -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Os")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${LINKER_SCRIPT}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${CMAKE_BINARY_DIR}/${PROG_NAME}.map")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --specs=nano.specs --specs=nosys.specs")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--cref -Wl,--check-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--warn-common -Wl,--warn-section-align")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--defsym=_init=0 -Wl,--defsym=_fini=0")

# Add MCU flags to all compilers
add_compile_options(${MCU_FLAGS})
add_link_options(${MCU_FLAGS})

# Core library include directories
set(CORE_INCLUDES
    ${LIB_DIR}/samd-core/libraries/Adafruit_TinyUSB_Arduino/src
    ${LIB_DIR}/samd-core/libraries/Adafruit_ZeroDMA
    ${LIB_DIR}/CMSIS/CMSIS/Core/Include
    ${LIB_DIR}/CMSIS/CMSIS/DSP/Include
    ${LIB_DIR}/CMSIS-Atmel/CMSIS-Atmel/CMSIS/Device/ATMEL
    ${LIB_DIR}/samd-core/cores/arduino
    ${LIB_DIR}/samd-core/libraries/Wire
    ${LIB_DIR}/samd-core/libraries/SPI
    ${PLATFORM_DIR}
    ${PLATFORM_DIR}/variant
)

# Create core library
add_subdirectory(third-party)

# Create main executable
add_subdirectory(src)

# Custom target for uploading via UF2 bootloader
# The Adafruit PyGamer M4 shows up as a USB drive when in bootloader mode
# Double-tap the reset button to enter bootloader mode
add_custom_target(upload
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tools/uf2/utils/uf2conv.py
            ${UNIFIED_BIN_DIR}/${PROG_NAME}.bin
            --base 0x4000
            --family 0x55114460
            --deploy
    DEPENDS ${PROG_NAME}.elf
    COMMENT "Uploading ${PROG_NAME}.uf2 to PyGamer M4 (ensure device is in bootloader mode)"
)

# Alternative upload target using pre-built UF2 file
add_custom_target(upload-uf2
    COMMAND ${CMAKE_COMMAND} -E copy
            ${UNIFIED_BIN_DIR}/${PROG_NAME}.uf2
            /media/${USER}/PYGAMERBOOT/firmware.uf2
    DEPENDS ${PROG_NAME}.elf
    COMMENT "Copying ${PROG_NAME}.uf2 to mounted PyGamer M4 bootloader drive"
)

# Custom target for resource baking
add_custom_target(bake
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tools/bake.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Baking resources from res/ directory"
)
