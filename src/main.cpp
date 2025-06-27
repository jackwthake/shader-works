/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#define ARDUINO_MAIN
#include "Arduino.h"

#include "Device.h"
#include "resource_manager.h"
#include "renderer.h"

#include "util/helpers.h"

// Initialize C library
extern "C" void __libc_init_array(void);

Transform camera;

resource_id_t cube_id;
resource_id_t atlas_id;
Model cube;


// Initialize board components
static void init_device_namespace() {
  using namespace Device;

  // Initialise hardware
  pin_init();
  tft_init();
  manager.init_QSPI_flash();
}


/**
 * Manages the double buffering rendering strategy
 */
void render_frame() {
  using namespace Device;

  // clear buffers and swap back and front buffers
  _swap_ptr(front_buffer, back_buffer);
  memset(back_buffer, 0x00, screen_buffer_len * sizeof(uint16_t));
  memset(depth_buffer, max_depth, screen_buffer_len * sizeof(float));

  render_model(back_buffer, camera, cube); // draw cube

  // Blit the framebuffer to the screen
  tft->drawRGBBitmap(0, 0, back_buffer, width, height);
}


void setup() {
  using Device::manager;

  // board initialization
  init_device_namespace();

  // load resources
  cube_id = manager.load_resource("cube.obj");
  if (cube_id == INVALID_RESOURCE)
    log_panic("Failed to load cube.obj");
   
  manager.read_obj_resource(cube_id, cube);
  if (cube.vertices.size() == 0)
    log_panic("Failed to init cube model");
  
  resource_id_t texture_id = manager.load_resource("atlas.bmp");
  if (texture_id != INVALID_RESOURCE) {
    cube.texture_atlas_id = texture_id; // Assign the texture ID to your model
  } else {
    log_panic("Invalid bitmap");
  }
  
  // now that we have the cube vertices in memory,
  // we doen't need the cube resource in memory (vertices won't change)
  if (manager.unload_resource(cube_id)) {
    Serial.println("Cube unloaded successfuilly");
  } else {
    Serial.println("Cube unloaded unsuccessfuilly");
  }
  
  compute_uv_coords(cube.uvs, 2); // top face
  compute_uv_coords(cube.uvs, 0); // bottom face
  compute_uv_coords(cube.uvs, 1); // side face
  compute_uv_coords(cube.uvs, 1); // side face
  compute_uv_coords(cube.uvs, 1); // side face
  compute_uv_coords(cube.uvs, 1); // side face

  
  cube.transform.position = { 1, 2, 5 };
  cube.transform.yaw = DEG_TO_RAD * 45;
  cube.transform.pitch = DEG_TO_RAD * 45;
  camera.position = { 1, 0, 1 };

  Device::log_debug_to_screen = false;
}


void loop() {
  using namespace Device;

  now = millis();

  // Process all pending ticks to maintain a fixed timestep
  int ticks_processed = 0;
  while ((now - last_tick) >= tick_interval && ticks_processed < 5) { // Limit catch-up to avoid spiral of death
    delta_time = tick_interval / 1000.0f; // fixed delta time in seconds
    last_tick += tick_interval;

    camera.yaw += -(read_joystick_x() * 2) * delta_time;

    float3 cam_right, cam_up, cam_fwd, move_delta = { 0.0, 0.0, 0.0 };
    camera.get_basis_vectors(cam_right, cam_up, cam_fwd);

    if (read_joystick_y() < -JOYSTICK_THRESH)
      move_delta += cam_fwd; // (read_joystick_y() * 5) * delta_time;
    else if (read_joystick_y() > JOYSTICK_THRESH)
      move_delta += (cam_fwd * -1);
    
    // bob and spin test cube
    cube.transform.position.y = sin((float)now / 1000) * 7 * delta_time;
    cube.transform.yaw += delta_time;


    /** Button Test */
    uint32_t buttons = read_buttons();
    if (buttons & BUTTON_MASK_A) {
      camera.position.y += 2 * delta_time;
    } else if (buttons & BUTTON_MASK_B) {
      camera.position.y -= 2 * delta_time;
    } else if (buttons & BUTTON_MASK_START) { // Strafe
      move_delta += cam_right;
    } if (buttons & BUTTON_MASK_SELECT) {
      move_delta += cam_right * -1;
    }

    camera.position += float3::normalize(move_delta) * 2 * delta_time;
    ticks_processed++;
  }

  /* Update back buffer, swap buffers and blit framebuffer to screen */
  render_frame();
}


int main(void) {
  init();                   // Arduino.h board initialization
  __libc_init_array();      // initialize libc

  delay(1);                 // Let the dust settle
  USBDevice.init();         // Startup USB Device
  USBDevice.attach();  

  Serial.begin(115200);
  while (!Serial) { delay(50); }

  setup();

  for (;;) {
    loop();

    delay(1);
    yield();                // yield run usb background task
    if (serialEventRun) {   // if theres a serial event, call the handler in Arduino.h
      serialEventRun();
    }
  }

  return 0;
}
