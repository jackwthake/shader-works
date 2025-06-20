#include <Arduino.h>

#include "src/device.h"
#include "src/util/helpers.h"
#include "src/renderer.h"
#include "src/util/model.h"

#include "src/resource_manager.h"

Device::Resource_manager *res_manager = NULL;

/**
 * Global vars for test model
 */
std::string cube_obj;
std::vector<float3> cube_vertices;
Model cube, cube2;


void setup() {
  Serial.begin(115200);
  while(!Serial) {
    // Wait for serial to be ready
  }

  // Initialise hardware
  Device::pin_init();
  Device::tft_init();
  Device::Resource_manager *res_manager = new Device::Resource_manager();
  
  log("Initialized.\n");

  // load cube model
  int idx = res_manager->load_resource("cube.obj");
  Device::Resource *res = res_manager->get_resource(idx);
  if (!res) {
    log_panic("Failed to load cube.obj");
  }

  cube_vertices = read_obj(std::string(res->data));

  // Initialize the cube model
  cube.vertices = cube_vertices;
  cube2.vertices = cube_vertices;

  cube.cols = {
      random_color(), random_color(), random_color(), random_color(),
      random_color(), random_color(), random_color(), random_color(),
      random_color(), random_color(), random_color(), random_color()
  };

  cube2.cols = {
      random_color(), random_color(), random_color(), random_color(),
      random_color(), random_color(), random_color(), random_color(),
      random_color(), random_color(), random_color(), random_color()
  };

  cube.transform.position = { 0, 0, 5 };
  cube2.transform.position = {-2, -2, 8};

  delay(500);
  Device::last_tick = millis();
  Device::log_debug_to_screen = false;
}


/**
 * Manages the double buffering rendering strategy
 */
void render_frame() {
  using namespace Device;

  _swap_ptr(front_buffer, back_buffer);
  memset(back_buffer, 0x00, screen_buffer_len * sizeof(uint16_t));
  memset(depth_buffer, max_depth, screen_buffer_len * sizeof(float));

  render_model(back_buffer, cube);
  render_model(back_buffer, cube2);

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

    cube.transform.position.x += (read_joystick_x() * 5) * delta_time;
    cube.transform.position.y += (read_joystick_y() * 5) * delta_time;
  
    cube2.transform.position.y = sin((millis() / 75)  * delta_time) * 5;
    cube2.transform.yaw += 2 * delta_time;

    /** Button Test */
    uint32_t buttons = read_buttons();
    if (buttons & BUTTON_MASK_A) {
      log("A Pressed\n");
    } else if (buttons & BUTTON_MASK_B) {
      log("B Pressed\n");
    } else if (buttons & BUTTON_MASK_START) {
      log("Start Pressed\n");
    } if (buttons & BUTTON_MASK_SELECT) {
      log("Select Pressed\n");
    }

    ticks_processed++;
  }

  /* Update back buffer, swap buffers and blit framebuffer to screen */
  render_frame();
}