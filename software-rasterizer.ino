#include <Arduino.h>

#include <cstring>

#include "src/device.h"
#include "src/resource_manager.h"
#include "src/renderer.h"

#include "src/util/helpers.h"

#define BAUD_RATE 115200

Transform camera;

resource_id_t cube_id;
resource_id_t atlas_id;
Model cube;


// Wait for serial connection
static void wait_for_serial() {
  Serial.begin(BAUD_RATE);
  while(!Serial) { delay(75); }
}


// Initialize board components
static void init_device_namespace() {
  using namespace Device;

  // Initialise hardware
  pin_init();
  tft_init();
  manager.init_QSPI_flash();
}


void compute_uv_coords(std::vector<float2>& uvs, int tile_id) {
  if (tile_id < 0 || tile_id >= TOTAL_TILES) {
    Serial.printf("Error: Tile ID %d is out of bounds. Must be between 0 and %d\n", tile_id, (TOTAL_TILES - 1));
  }

  // Calculate the tile's top-left pixel corner in the atlas.
  float tile_x_start = static_cast<float>((tile_id % TILES_PER_ROW) * TILE_WIDTH_PX);
  float tile_y_start = static_cast<float>((tile_id / TILES_PER_ROW) * TILE_HEIGHT_PX);

  // Convert pixel coordinates to normalized UV coordinates [0.0, 1.0] by casting constants to float.
  float u_start = tile_x_start / static_cast<float>(ATLAS_WIDTH_PX);
  float v_start = tile_y_start / static_cast<float>(ATLAS_HEIGHT_PX);
  float u_end = (tile_x_start + static_cast<float>(TILE_WIDTH_PX)) / static_cast<float>(ATLAS_WIDTH_PX);
  float v_end = (tile_y_start + static_cast<float>(TILE_HEIGHT_PX)) / static_cast<float>(ATLAS_HEIGHT_PX);

  // Define the 4 UV corners for the tile.
  float2 uv_top_left     = {u_start, v_start};
  float2 uv_top_right    = {u_end,   v_start};
  float2 uv_bottom_left  = {u_start, v_end};
  float2 uv_bottom_right = {u_end,   v_end};

  // Add UVs for the first triangle (v0, v1, v2)
  uvs.push_back(uv_top_left);     // Corresponds to vertex v0
  uvs.push_back(uv_bottom_left);  // Corresponds to vertex v1
  uvs.push_back(uv_bottom_right); // Corresponds to vertex v2

  // Add UVs for the second triangle (v0, v2, v3)
  uvs.push_back(uv_top_left);     // Corresponds to vertex v0
  uvs.push_back(uv_bottom_right); // Corresponds to vertex v2
  uvs.push_back(uv_top_right);    // Corresponds to vertex v3
}


#ifdef RUN_BOARD_TESTS
/**
 * Test main, Initialize board then run_tests
 */
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

  // board initialization
  wait_for_serial();
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

  cube.transform.position = { 0, 0, 5 };
  Device::log_debug_to_screen = false;
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

#endif // end if !RUN_BOARD_TESTS