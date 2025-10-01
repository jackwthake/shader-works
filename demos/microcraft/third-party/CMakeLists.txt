# Core library build for SAMD51

# Collect all source files for the core library
file(GLOB_RECURSE CORE_SOURCES
    # SAMD Core Arduino sources (if submodules are initialized)
    ${LIB_DIR}/samd-core/cores/arduino/*.c
    ${LIB_DIR}/samd-core/cores/arduino/*.cpp
    ${LIB_DIR}/samd-core/cores/arduino/avr/*.c
    ${LIB_DIR}/samd-core/cores/arduino/avr/*.cpp
    ${LIB_DIR}/samd-core/cores/arduino/USB/*.c
    ${LIB_DIR}/samd-core/cores/arduino/USB/*.cpp

    # Libraries
    ${LIB_DIR}/samd-core/libraries/Wire/*.c
    ${LIB_DIR}/samd-core/libraries/Wire/*.cpp
    ${LIB_DIR}/samd-core/libraries/SPI/*.c
    ${LIB_DIR}/samd-core/libraries/SPI/*.cpp
    ${LIB_DIR}/samd-core/libraries/Adafruit_ZeroDMA/*.c
    ${LIB_DIR}/samd-core/libraries/Adafruit_ZeroDMA/*.cpp

    # Platform-specific files (variant, device, display, input)
    ${PLATFORM_DIR}/variant/*.c
    ${PLATFORM_DIR}/variant/*.cpp
    ${PLATFORM_DIR}/*.c
    ${PLATFORM_DIR}/*.cpp
)

# Exclude main.cpp from core library (it's in the application)
list(FILTER CORE_SOURCES EXCLUDE REGEX "main\\.cpp$")

# Exclude math_helper.c as it requires CMSIS-DSP library
list(FILTER CORE_SOURCES EXCLUDE REGEX "math_helper\\.[ch]$")

# Create the core library (or interface library if no sources found)
if(CORE_SOURCES)
    add_library(core_lib STATIC ${CORE_SOURCES})
    target_compile_options(core_lib PRIVATE
        -Wall
        -Wno-expansion-to-defined
    )
else()
    message(WARNING "No core library sources found. Creating interface library.")
    message(WARNING "If using git submodules, run: git submodule update --init --recursive")
    add_library(core_lib INTERFACE)
endif()

# Set include directories for the core library
target_include_directories(core_lib PUBLIC
    ${CORE_INCLUDES}
    ${SRC_DIR}
)

# Set compile options for the core library
target_compile_options(core_lib PRIVATE
    -Wall
    -Wno-expansion-to-defined
)
