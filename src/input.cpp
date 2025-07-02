#include "device.h"

#include <arduino.h>
#include <variant.h>


/**
 * Maps a value from the range [-512, 512] to [-1, 1].
 */
static float map_joystick_range(int16_t value) {
    // Clamp value to [-512, 512] just in case
    if (value < -512) value = -512;
    if (value > 512) value = 512;
    return static_cast<float>(value) / 512.0f;
}


namespace Device {

/**
 * Generic function that reads a joystick
 * @param pin The Analog pin to read
 * @param samples The number of samples to average
 * @returns a mapped vakue from -1 to 1
*/
static float read_joystick(pins pin, uint8_t samples) {
  float reading = 0;

  for (int i = 0; i < samples; i++) {
    reading += analogRead(pin);
  }
  
  reading /= samples;
  reading -= 512; // adjust range from 0->1024 to -512 to 511;

  float mapped = map_joystick_range(reading);
  if (mapped > JOYSTICK_THRESH || mapped < -JOYSTICK_THRESH)
    return mapped;
  
  return 0.0;
}


/**
 * Reads the X axis of the joystick, averages over the given number of samples.
 * @param sampling Number of samples to average (default: 3)
 * @return Averaged X axis value, centered to range [-512, 511]
*/
float read_joystick_x(uint8_t sampling) {
  return read_joystick(JOYSTICK_PIN_X, sampling);
}


/**
 * Reads the Y axis of the joystick, averages over the given number of samples.
 * @param sampling Number of samples to average (default: 3)
 * @return Averaged Y axis value, centered to range [-512, 511]
*/
float read_joystick_y(uint8_t sampling) {
  return read_joystick(JOYSTICK_PIN_Y, sampling);
}


/**
 * Reads the state of the buttons connected to the device.
 * Utilizes shift register to read multiple button states sequentially.
 * Maps the button states to predefined button masks.
 * 
 * @return A 32-bit integer representing the state of the buttons, 
 *         where each bit corresponds to a specific button mask.
 */
uint32_t read_buttons() {
  uint32_t buttons = 0;

  uint8_t shift_buttons = 0;
  digitalWrite(BUTTON_PIN_LATCH, LOW);
  delayMicroseconds(1);
  digitalWrite(BUTTON_PIN_LATCH, HIGH);
  delayMicroseconds(1);

  for (int i = 0; i < 8; i++) {
    shift_buttons <<= 1;
    shift_buttons |= digitalRead(BUTTON_PIN_DATA);
    digitalWrite(BUTTON_PIN_CLOCK, HIGH);
    delayMicroseconds(1);
    digitalWrite(BUTTON_PIN_CLOCK, LOW);
    delayMicroseconds(1);
  }

  if (shift_buttons & BUTTON_SHIFT_B)
    buttons |= BUTTON_MASK_B;
  if (shift_buttons & BUTTON_SHIFT_A)
    buttons |= BUTTON_MASK_A;
  if (shift_buttons & BUTTON_SHIFT_SELECT)
    buttons |= BUTTON_MASK_SELECT;
  if (shift_buttons & BUTTON_SHIFT_START)
    buttons |= BUTTON_MASK_START;

  return buttons;
}

} // namespace Device