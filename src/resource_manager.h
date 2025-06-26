#pragma once

#include <stdint.h>
#include <stddef.h>

#include <Adafruit_SPIFlash.h>
#include "util/model.h"

typedef int resource_id_t;
constexpr resource_id_t INVALID_RESOURCE = -1;

// --- Constants for the Atlas ---
constexpr int ATLAS_WIDTH_PX = 80;
constexpr int ATLAS_HEIGHT_PX = 24;
constexpr int TILE_WIDTH_PX = 8;
constexpr int TILE_HEIGHT_PX = 8;

// Derived constants
constexpr int TILES_PER_ROW = ATLAS_WIDTH_PX / TILE_WIDTH_PX;
constexpr int TILES_PER_COL = ATLAS_HEIGHT_PX / TILE_HEIGHT_PX;
constexpr int TOTAL_TILES = TILES_PER_ROW * TILES_PER_COL;
constexpr int TILE_PIXEL_COUNT = TILE_WIDTH_PX * TILE_HEIGHT_PX;


enum resource_type_t {
  BITMAP_FILE,
  DATA_FIlE,
  INVALID_FILE
};


struct Resource_entry_t {
  void *data;
  resource_type_t type;
  
  uint32_t length;
  uint16_t width, height;
};


/**
 * Simple readonly resource manager. Used to load bitmaps and data files from onboard flash
 */
class Resource_manager {
public:
  Resource_manager();
  ~Resource_manager();
  void init_QSPI_flash();

  resource_id_t load_resource(const char *path);
  const Resource_entry_t *get_resource(resource_id_t id);
  bool unload_resource(resource_id_t id);
  
  bool read_obj_resource(resource_id_t id, Model &model);
  bool get_tile_from_atlas(resource_id_t id, unsigned tile_id, uint16_t *output_tile_data);
private:

  bool load_text(File32 &f);
  bool load_bmp(File32 &f);

  static constexpr int8_t MAX_RESOURCES = 5;
  Resource_entry_t resources[MAX_RESOURCES];
  resource_id_t num_resources;

  Adafruit_FlashTransport_QSPI flash_transport;
  Adafruit_SPIFlash flash;
  FatVolume file_sys;
};