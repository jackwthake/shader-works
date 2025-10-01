#pragma once

#include <stdint.h>

#include <variant.h> // For pin definitions

// Encapsulate necessary program globals
namespace Device  {
  constexpr float JOYSTICK_THRESH = 0.2;

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