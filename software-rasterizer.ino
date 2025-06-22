#include <Arduino.h>

#include <cstring>

#include "src/device.h"
#include "src/resource_manager.h"
#include "src/renderer.h"

#include "src/util/helpers.h"

#define BAUD_RATE 115200

resource_id_t cube_id;
Model *cube;


static void wait_for_serial() {
  Serial.begin(BAUD_RATE);
  while(!Serial) { delay(75); }
}


static void init_device_namespace() {
  using namespace Device;

  // Initialise hardware
  pin_init();
  tft_init();
  manager.init_QSPI_flash();
}


#ifdef RUN_BOARD_TESTS
#include "src/test/test.h"

void setup() {
  wait_for_serial();
  init_device_namespace();

  Serial.println("Running in unit test mode.\n\n");
  run_tests();

  while (1) { delay(50); }
}

void loop() {}
#else // else if !RUN_BOARD_TESTS

void setup() {
  using Device::manager;

  wait_for_serial();
  init_device_namespace();

  cube_id = manager.load_resource("cube.obj");
  if (cube_id == -1)
    log_panic("Failed to load cube.obj");
  
  cube = manager.read_obj_resource(cube_id);
  if (!cube)
    log_panic("Failed to init cube model");
  
  cube->cols = {
    random_color(), random_color(), random_color(), random_color(),
    random_color(), random_color(), random_color(), random_color(),
    random_color(), random_color(), random_color(), random_color()
  };

  cube->transform.position = { 0, 0, 5 };
}


/**
 * Manages the double buffering rendering strategy
 */
void render_frame() {
  using namespace Device;

  _swap_ptr(front_buffer, back_buffer);
  memset(back_buffer, 0x00, screen_buffer_len * sizeof(uint16_t));
  memset(depth_buffer, max_depth, screen_buffer_len * sizeof(float));

  render_model(back_buffer, *cube);

  // Blit the framebuffer to the screen
  tft->drawRGBBitmap(0, 0, back_buffer, width, height);
}


void loop() {
  using namespace Device;

  now = millis();

  // Process all pending ticks to maintain a fixed timestep
  int ticks_processed = 0;
  while ((now - last_tick) >= tick_interval && ticks_processed < 5) { // Limit catch-up to avoid spiral of death
    delta_time = tick_interval / 1000.0f; // fixed delta time in seconds
    last_tick += tick_interval;

    cube->transform.position.x += (read_joystick_x() * 5) * delta_time;
    cube->transform.position.y += (read_joystick_y() * 5) * delta_time;

    /** Button Test */
    uint32_t buttons = read_buttons();
    if (buttons & BUTTON_MASK_A) {
      cube->transform.yaw += 5 * delta_time;
      log("A Pressed\n");
    } else if (buttons & BUTTON_MASK_B) {
      cube->transform.yaw -= 5 * delta_time;
      log("B Pressed\n");
    } else if (buttons & BUTTON_MASK_START) {
      cube->transform.pitch += 5 * delta_time;
      log("Start Pressed\n");
    } if (buttons & BUTTON_MASK_SELECT) {
      cube->transform.pitch -= 5 * delta_time;
      log("Select Pressed\n");
    }

    ticks_processed++;
  }

  /* Update back buffer, swap buffers and blit framebuffer to screen */
  render_frame();
}

#endif // end if !RUN_BOARD_TESTS