#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "resource_manager.h"

// Encapsulate necessary program globals
namespace Device  {
  constexpr int width = 160, height = 128;
  constexpr int screen_buffer_len = width * height;
  constexpr float max_depth = 100.0f; // Maximum depth value for the depth buffer
  constexpr unsigned long tick_interval = 50; // 20 ticks per second (1000ms / 20 = 50ms)
  constexpr float JOYSTICK_THRESH = 0.2;
  
  extern Resource_manager manager;

  extern uint16_t front_buffer[screen_buffer_len];
  extern uint16_t back_buffer[screen_buffer_len];
  extern float depth_buffer[screen_buffer_len];

  // Game Variables
  extern unsigned long now, last_tick;
  extern float delta_time;
  extern bool log_debug_to_screen;

  extern void pin_init();
  extern void tft_init();
  extern void spi_flash_init();

  extern float read_joystick_x(uint8_t sampling=3);
  extern float read_joystick_y(uint8_t sampling=3);
  extern uint32_t read_buttons();
  
  enum pins : uint32_t {
    TFT_CS = 44,                // Chip select for the TFT display
    TFT_DC = 45,                // Data/Command select for the TFT display
    TFT_RST = 46,               // Reset pin for the TFT display
    TFT_BACKLIGHT = 47,         // Backlight control pin for the TFT display

    JOYSTICK_PIN_X = A11,       // X Potentiometer from joystick
    JOYSTICK_PIN_Y = A10,       // y Potentiometer from joystick

    BUTTON_PIN_CLOCK = 48,      // Used to pull data for A, B, SELECT, START
    BUTTON_PIN_DATA = 49,
    BUTTON_PIN_LATCH = 50,

    BUTTON_MASK_A = 0x01,       // Button masks to decode input
    BUTTON_MASK_B = 0x02,
    BUTTON_MASK_SELECT = 0x04,
    BUTTON_MASK_START = 0x08,

    BUTTON_SHIFT_B = 0x80,      // Used internally to decode button data
    BUTTON_SHIFT_A = 0x40,
    BUTTON_SHIFT_START = 0x20,
    BUTTON_SHIFT_SELECT = 0x10,

    RUMBLE_PIN = A0,            // Runble Motor pin
    BATT_SENSOR = A6            // Battery Level Sensor
  };
};