#define ARDUINO_MAIN
#include <Arduino.h>

#include "display.h"
#include "device.h"
#include "resource_manager.h"
#include "renderer.h"

#include "util/helpers.h"

// Initialize C library
extern "C" void __libc_init_array(void);

constexpr size_t MAX_CUBES = 16; // Maximum number of cubes to render
Model cubes[MAX_CUBES] = { Model() };

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


static void button_init() {
  using namespace Device;

  pinMode(BUTTON_PIN_CLOCK, OUTPUT);
  digitalWrite(BUTTON_PIN_CLOCK, HIGH);
  
  pinMode(BUTTON_PIN_LATCH, OUTPUT);
  digitalWrite(BUTTON_PIN_LATCH, HIGH);

  pinMode(BUTTON_PIN_DATA, INPUT);
}


void resource_init(Resource_manager &manager, resource_id_t &cube_id, resource_id_t &atlas_id) {
  manager.init_QSPI_flash();
  
  // Load the cube model from resources
  cube_id = manager.load_resource("cube.obj");
  if (cube_id == INVALID_RESOURCE) {
    Serial.println("Failed to load cube model.\n");
    for(;;); // Halt execution if the cube model cannot be loaded
  }

  // Load the texture atlas
  atlas_id = manager.load_resource("atlas.bmp");
  if (atlas_id == INVALID_RESOURCE) {
    Serial.println("Invalid bitmap");
    for(;;); // Halt execution if the cube model cannot be loaded
  }
  
  Serial.println("Resources initialized successfully.");
}


void init_cubes(Resource_manager &manager, resource_id_t cube_id, resource_id_t atlas_id) {
  Serial.println("Starting cube initialization...");
  
  std::vector<float3> temp_vertices;

  // Load vertex data for each cube
  Serial.println("Loading OBJ resource...");
  bool success = manager.read_obj_resource(cube_id, temp_vertices);
  if (!success || temp_vertices.size() == 0) {
    Serial.printf("Failed to init cube model, vertices count: %d\n", temp_vertices.size());
    for(;;); // Halt execution if the cube model cannot be initialized
  }
  
  Serial.printf("Loaded %d vertices successfully\n", temp_vertices.size());

  Serial.println("Initializing cube models...");
  for (int i = 0; i < MAX_CUBES; ++i) {
    cubes[i] = Model();
    cubes[i].transform.position = { (float)(i % 4) * 2.0f, -1, (float)(i / 4) * 2.0f };
    cubes[i].transform.yaw = 0.0f;
    cubes[i].transform.pitch = 0.0f;
    cubes[i].texture_atlas_id = atlas_id;
    cubes[i].vertices = temp_vertices;

    compute_uv_coords(cubes[i].uvs, 2); // top face
    compute_uv_coords(cubes[i].uvs, 0); // bottom face
    compute_uv_coords(cubes[i].uvs, 1); // side face
    compute_uv_coords(cubes[i].uvs, 1); // side face
    compute_uv_coords(cubes[i].uvs, 1); // side face
    compute_uv_coords(cubes[i].uvs, 1); // side face
  }

  Serial.println("Cubes initialized successfully.");
}

/**
 * Manages the double buffering rendering strategy
 */
void render_frame(Resource_manager &manager, uint16_t *buf, float *depth_buffer, Display &display, Transform &camera) {
  Serial.println("Starting buffer clear...");
  // clear buffers and swap back and front buffers
  memset(buf, 0x971c, display.width * display.height * sizeof(uint16_t));

  Serial.println("Clearing depth buffer...");
  for (int i = 0; i < Display::width * Display::height; ++i) {
    depth_buffer[i] = MAX_DEPTH;
  }

  for (int i = 0; i < MAX_CUBES; ++i) {
    Serial.printf("Rendering cube %d...\n", i);
    render_model(buf, depth_buffer, manager, camera, cubes[i]); // draw cubes
    Serial.printf("Cube %d rendered\n", i);
  }

  // Blit the framebuffer to the screen
  display.draw(buf);
}


void tick_world(float delta_time, Transform &camera) {
  using namespace Device;

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
}


int main(void) {
  constexpr unsigned long tick_interval = 50;
  unsigned long now, last_tick = 0;
  float delta_time = 0.0f;

  resource_id_t cube_id;
  resource_id_t atlas_id;
  Transform camera;

  init_arduino(); // Initialize Arduino and USB
  button_init();  // Initialize button pins
  
  // Initialize display and buffers
  Display display;
  uint16_t screen_buffer[Display::width * Display::height];
  float depth_buffer[Display::width * Display::height];
  
  Serial.println("Initializing display...");
  display.begin();
  Serial.println("Display initialized successfully. Starting render loop.");
  
  // Initialize resources
  Resource_manager manager;
  resource_init(manager, cube_id, atlas_id);
  init_cubes(manager, cube_id, atlas_id); // Initialize cubes with the texture atlas
  
  camera.position = { 1, 2, 0 };
  camera.yaw = 0.0f; // Set initial yaw to 0
  camera.pitch = 0.0f; // Set initial pitch to 0

  Serial.println("Camera initialized");

  last_tick = millis(); // Initialize last tick time
  Serial.println("Starting main loop...");

  for (;;) {
    serial_poll_events(); // Poll for serial events
    now = millis();

    // Process all pending ticks to maintain a fixed timestep
    int ticks_processed = 0;
    while ((now - last_tick) >= tick_interval && ticks_processed < 5) { // Limit catch-up to avoid spiral of death
      delta_time = (float)tick_interval / 1000.0f; // fixed delta time in seconds
      last_tick += tick_interval;

      tick_world(delta_time, camera); // Update the world state
      ticks_processed++;
    }

    render_frame(manager, screen_buffer, depth_buffer, display, camera); // Render the frame
  }

  return 0;
}