#include "test.h"

#include "../device.h"
#include "../util/helpers.h"
#include "expected_values.h"

static void text_load_test() {
  using Device::manager;

  Serial.println("Text file load test: load cube.obj");

  resource_id_t cube_id = manager.load_resource("cube.obj");
  if (cube_id < 0)
    log_panic("Failed to load cube.obj");
  
  if (cube_id != 0)
    log_panic("Unexpected cube id: %d Expected 0\n", cube_id);
  
  const Resource_entry_t *cube = manager.get_resource(cube_id);
  if (!cube)
    log_panic("Failed to retrieve cube resource at id %d\n", cube_id);
  if (cube->length == 0)
    log_panic("Resource entry length is zero");
  if (cube->data == nullptr)
    log_panic("Resource entry data is null");
  if (cube->type == INVALID_FILE)
    log_panic("Resource entry type is INVALID_FILE");

  Serial.printf("Cube: id: %d, length: %u\n", cube_id, cube->length, cube->data);
  Serial.println(reinterpret_cast<char*>(cube->data));
  if (strcmp(reinterpret_cast<char*>(cube->data), cube_obj_expected) != 0) {
    Serial.println("cube obj file contents do not match.");
    Serial.println("\nText file load test FAILED");
  } else {
    Serial.println("cube obj file contents match.");
    Serial.println("\nText file load test SUCCEEDED");
  }
}

static void bitmap_test() {
  using Device::manager;
  using Device::tft;

  Serial.println("Bitmap file load test: load test.bmp");

  resource_id_t bmp_id = manager.load_resource("test.bmp");
  if (bmp_id < 0)
    log_panic("Failed to load atlas.bmp");
  
  if (bmp_id != 1)
    log_panic("Unexpected cube id: %d Expected 1\n", bmp_id);
  
  const Resource_entry_t *bmp = manager.get_resource(bmp_id);
  if (!bmp)
    log_panic("Failed to retrieve cube resource at id %d\n", bmp_id);
  if (bmp->length == 0)
    log_panic("Resource entry length is zero");
  if (bmp->length != 8 * 8)
    log_panic("Bitmap is wrong size");
  if (bmp->data == nullptr)
    log_panic("Resource entry data is null");
  if (bmp->type == INVALID_FILE)
    log_panic("Resource entry type is INVALID_FILE");
  if (bmp->type != BITMAP_FILE)
    log_panic("Resource entry type should be BITMAP_FILE");

  Serial.println("Bitmap successfully loaded");
  Serial.println("Comparing to expected");
  if (memcmp(bmp->data, bitmap_expected, bmp->length) != 0) {
    log_panic("bitmap data does not match expected data.\n");
  }

  Serial.println("Memory matches expected values");
  Serial.println("executing draw call");
  tft->drawRGBBitmap(0, 0, (uint16_t *)bmp->data, 8, 8);
}

void run_tests() {
  Serial.println("Begin resource manager test\n");
  Serial.println("==============================");


  text_load_test();

  Serial.println("==============================");
  bitmap_test();

  Serial.println("==============================");
  Serial.println("Tests complete.");
}
