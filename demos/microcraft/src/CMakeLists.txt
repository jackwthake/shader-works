# Main project source files

# Collect all application source files
file(GLOB_RECURSE APP_SOURCES
    ${SRC_DIR}/*.c
    ${SRC_DIR}/*.cpp
)

# Add platform-specific main.cpp
list(APPEND APP_SOURCES ${PLATFORM_DIR}/main.cpp)

# Create the executable
add_executable(${PROG_NAME}.elf ${APP_SOURCES})

# Include directories for the application
target_include_directories(${PROG_NAME}.elf PRIVATE
    ${SRC_DIR}
    ${CORE_INCLUDES}
)

# Link the core library and math libraries
target_link_libraries(${PROG_NAME}.elf PRIVATE
    core_lib
    m
    gcc
    c
    nosys
)

# Post-build commands to generate .bin, .hex, and .uf2 files
add_custom_command(TARGET ${PROG_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROG_NAME}.elf> ${CMAKE_BINARY_DIR}/${PROG_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom $<TARGET_FILE:${PROG_NAME}.elf> ${CMAKE_BINARY_DIR}/${PROG_NAME}.hex
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/../tools/uf2/utils/uf2conv.py
            ${CMAKE_BINARY_DIR}/${PROG_NAME}.bin
            --base 0x4000
            --family 0x55114460
            --output ${CMAKE_BINARY_DIR}/${PROG_NAME}.uf2
    COMMENT "Generating ${PROG_NAME}.bin, ${PROG_NAME}.hex, and ${PROG_NAME}.uf2"
)

# Print size information
add_custom_command(TARGET ${PROG_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROG_NAME}.elf>
    COMMENT "Size information:"
)
