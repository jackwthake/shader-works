#include <Arduino.h>

#include <cstring>

#include "src/device.h"
#include "src/resource_manager.h"

#define BAUD_RATE 115200
#define WAIT_FOR_
#define RUN_BOARD_TESTS

static void wait_for_serial() {
  Serial.begin(115200);
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
  wait_for_serial();
  init_device_namespace();

  while (1) { delay(50); }
}

void loop() {

}

#endif // end if !RUN_BOARD_TESTS