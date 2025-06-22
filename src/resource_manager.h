#pragma once

#include <stdint.h>
#include <stddef.h>

#include <Adafruit_SPIFlash.h>
#include "util/model.h"

typedef int resource_id_t;
constexpr resource_id_t INVALID_RESOURCE = -1;


enum resource_type_t {
  BITMAP_FILE,
  DATA_FIlE,
  INVALID_FILE
};


struct Resource_entry_t {
  void *data;
  resource_type_t type;
  uint32_t length;
};


class Resource_manager {
public:
  Resource_manager();
  ~Resource_manager();
  void init_QSPI_flash();

  resource_id_t load_resource(const char *path);
  const Resource_entry_t *get_resource(resource_id_t id);
  
  Model *read_obj_resource(resource_id_t id);
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