#define ARDUINO_MAIN
#include <Arduino.h>
#include <unistd.h>

#include "display.h"
#include "device.h"
#include "renderer.h"

#include "scene.h"

// Initialize C library
extern "C" void __libc_init_array(void);

// extern "C" {
//   extern char __data_start__;   // Start of .data section
//   extern char __data_end__;     // End of .data section
//   extern char __bss_start__;    // Start of .bss section
//   extern char __bss_end__;      // End of .bss section
//   extern void *end;             // Top of the heap upon initialization (end of .bss)
//   extern void *__StackTop;      // Top of the stack (highest RAM address)
// }

// /**
//  * @brief Prints a detailed report of the system's current RAM usage.
// */
// void printMemoryUsage() {
//   // A pointer to the current top of the heap. sbrk(0) is a standard C call to get this.
//   char* heap_top = (char*)sbrk(0);

//   // A variable on the stack. Its address is the current stack pointer.
//   char stack_ptr_var;
//   char* stack_ptr = &stack_ptr_var;

//   // --- Calculate Sizes ---
  
//   // Static data is split into .data (initialized globals) and .bss (zero-initialized globals).
//   size_t static_data_size = (&__data_end__ - &__data_start__) + (&__bss_end__ - &__bss_start__);

//   // The heap starts at the end of the .bss section and extends to its current top.
//   size_t heap_size = heap_top - (char*)&end;

//   // The stack starts at the top of RAM and grows downwards.
//   // The used stack is the space between the top of RAM and the current stack pointer.
//   size_t stack_used = (char*)&__StackTop - stack_ptr;
  
//   // Total RAM used is the sum of all segments.
//   size_t ram_used = static_data_size + heap_size + stack_used;

//   // Total RAM available on the chip.
//   size_t ram_total = (char*)&__StackTop - &__data_start__;

//   // --- Print Report ---
//   Serial.println(F("--- System Memory Report ---"));
//   Serial.printf("Static Data (.data + .bss): %u bytes\n", static_data_size);
//   Serial.printf("Heap Used                 : %u bytes\n", heap_size);
//   Serial.printf("Stack Used (Current)      : %u bytes\n", stack_used);
//   Serial.println(F("------------------------------"));
//   Serial.printf("Total RAM Used            : %u bytes\n", ram_used);
//   Serial.printf("Total RAM Available       : %u bytes\n", ram_total);
//   Serial.printf("Free RAM (Heap + Stack)   : %u bytes\n", ram_total - ram_used);
//   Serial.println(F("------------------------------"));
// }

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
void render_frame(Scene &scene, uint16_t *buf, float *depth_buffer, Display &display) {
  // clear buffers and swap back and front buffers
  memset(buf, 0x971c, display.width * display.height * sizeof(uint16_t));

  for (int i = 0; i < Display::width * Display::height; ++i) {
    depth_buffer[i] = MAX_DEPTH;
  }

  // Draw scene
  scene.render(buf, depth_buffer);  

  // Blit the framebuffer to the screen
  display.draw(buf);
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

  // printMemoryUsage(); // Print memory usage report
  
  Scene scene; // Create a scene instance
  Serial.println("Initializing display...");
  display.begin();
  Serial.println("Display initialized successfully. Starting render loop.");

  // printMemoryUsage(); // Print memory usage after display initialization

  scene.init(); // Initialize the scene

  // printMemoryUsage(); // Print memory usage after scene initialization

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