#define ARDUINO_MAIN
#include <Arduino.h>
#include <unistd.h>

#include "drivers/display.hpp"
#include "device.hpp"
#include "renderer.h"

#include "scene.h"

// Initialize C library
extern "C" void __libc_init_array(void);

// Required pixel conversion fucntions
u32 rgb_to_u32(u8 r, u8 g, u8 b) {
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);
  return SDL_MapRGBA(format, NULL, r, g, b, 255);
}

void u32_to_rgb(u32 color, u8 *r, u8 *g, u8 *b) {
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);
  SDL_GetRGB(color, format, NULL, r, g, b);
}

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

/**
 * Manages the double buffering rendering strategy
 */
static void render_frame(Scene &scene, uint16_t *buf, float *depth_buffer, Display &display) {
  for (int i = 0; i < Display::width * Display::height; ++i) {
    depth_buffer[i] = MAX_DEPTH;
    buf[i] = 0xBE9C;
  }

  // Draw scene
  scene.render(buf, depth_buffer);

  // Blit the framebuffer to the screen
  display.draw(buf);
}

void update_timing(fps_controller_t &controller) {
  controller.delta_time = (float)(current_time - controller.last_frame_time) / (millis() / 1000);
  controller.last_frame_time = millis() / 1000;
  if (controller.delta_time > 0.1f) controller.delta_time = 0.1f;
}

void handle_input(fps_controller_t &controller, transform_t &camera) {

}

int main(void) {
  constexpr unsigned long tick_interval = 20; // Fixed timestep interval in milliseconds
  unsigned long now, last_tick = 0;
  float delta_time = 0.0f;

  init_arduino(); // Initialize Arduino and USB
  button_init();  // Initialize button pins

  // Initialize display and buffers
  Display display;
  uint16_t screen_buffer[Display::width * Display::height];
  float depth_buffer[Display::width * Display::height];

  Scene scene; // Create a scene instance
  Serial.println("Initializing display...");
  display.begin();
  Serial.println("Display initialized successfully. Starting render loop.");

  scene.init(); // Initialize the scene

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

      scene.update(delta_time); // Update the scene state
      ticks_processed++;
    }

    render_frame(scene, screen_buffer, depth_buffer, display);
  }

  return 0;
}
