#define ARDUINO_MAIN
#include <Arduino.h>

#include "display.h"
#include "device.h"
#include "resource_manager.h"
#include "renderer.h"

#include "util/helpers.h"

// Initialize C library
extern "C" void __libc_init_array(void);


/* Arduino initialization */
static void init_arduino(size_t serial_wait_timeout = 1000) {
  init();                   // Arduino.h board initialization
  __libc_init_array();      // initialize libc

  delay(1);                 // Let the dust settle
  USBDevice.init();         // Startup USB Device
  USBDevice.attach();  

  Serial.begin(115200);     // Initialize Serial at 115200 baud, with timeout
  while (!Serial && (serial_wait_timeout-- > 0)) { delay(50); }

  interrupts();            // Enable interrupts
  Serial.println("Arduino initialized successfully!");
}


/* Serial polling */
static void serial_poll_events() {
  delay(1);
  yield();                // yield run usb background task
  if (serialEventRun) {   // if theres a serial event, call the handler in Arduino.h
    serialEventRun();
  }
}


void resource_init(resource_id_t &cube_id, resource_id_t &atlas_id, Model &cube) {
  using namespace Device;

  manager.init_QSPI_flash();

  // Initialize the cube model
  cube = Model();
  cube.transform.position = { 1, 2, 5 };
  cube.transform.yaw = DEG_TO_RAD * 45;
  cube.transform.pitch = DEG_TO_RAD * 45;
  
  // Load the cube model from resources
  cube_id = manager.load_resource("cube.obj");
  if (cube_id == INVALID_RESOURCE) {
    log_panic("Failed to load cube model.\n");
  }

  manager.read_obj_resource(cube_id, cube);
  if (cube.vertices.size() == 0)
    log_panic("Failed to init cube model");
  
  // Load the texture atlas
  resource_id_t texture_id = manager.load_resource("atlas.bmp");
  if (texture_id != INVALID_RESOURCE) {
    cube.texture_atlas_id = texture_id; // Assign the texture ID to your model
  } else {
    log_panic("Invalid bitmap");
  }

  compute_uv_coords(cube.uvs, 2); // top face
  compute_uv_coords(cube.uvs, 0); // bottom face
  compute_uv_coords(cube.uvs, 1); // side face
  compute_uv_coords(cube.uvs, 1); // side face
  compute_uv_coords(cube.uvs, 1); // side face
  compute_uv_coords(cube.uvs, 1); // side face
  
  Serial.println("Resources initialized successfully.");
}


/**
 * Manages the double buffering rendering strategy
 */
void render_frame(Display &display, Transform &camera, Model &cube) {
  using namespace Device;

  // clear buffers and swap back and front buffers
  _swap_ptr(front_buffer, back_buffer);
  memset(back_buffer, 0x00, screen_buffer_len * sizeof(uint16_t));
  memset(depth_buffer, max_depth, screen_buffer_len * sizeof(float));

  render_model(back_buffer, camera, cube); // draw cube

  // Blit the framebuffer to the screen
  display.draw(back_buffer);
}


void tick_world(Transform &camera, Model &cube) {
  using namespace Device;

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
}


int main(void) {
  resource_id_t cube_id;
  resource_id_t atlas_id;
  Transform camera;
  Model cube;

  init_arduino();
  Device::pin_init(); // Initialize button pins

  // Create the buffer to hold the screen's pixel data
  uint16_t screen_buffer[Display::width * Display::height];

  // Create the display object
  Display display;

  Serial.println("Initializing display...");
  display.begin();
  Serial.println("Display initialized successfully. Starting render loop.");

  // Initialize resources
  resource_init(cube_id, atlas_id, cube);

  camera.position = { 1, 0, 1 };

  for (;;) {
    using namespace Device;

    serial_poll_events(); // Poll for serial events
    now = millis();

    // Process all pending ticks to maintain a fixed timestep
    int ticks_processed = 0;
    while ((now - last_tick) >= tick_interval && ticks_processed < 5) { // Limit catch-up to avoid spiral of death
      delta_time = tick_interval / 1000.0f; // fixed delta time in seconds
      last_tick += tick_interval;

      tick_world(camera, cube);

      ticks_processed++;
    }

    render_frame(display, camera, cube); // Render the frame
  }

  return 0;
}