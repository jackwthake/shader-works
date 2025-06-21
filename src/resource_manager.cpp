#include "resource_manager.h"

#include <Adafruit_SPIFlash.h>
#include <SdFat_Adafruit_Fork.h>

#include "renderer.h"
#include "util/helpers.h"


Resource::Resource() : res_type(INVALID_FILE), id(-1) {
  raw_data_size = 0;
  raw_data = nullptr;
}


Resource::Resource(unsigned id, RESOURCE_TYPE type, File32 &f) : id(id), res_type(type) {
  f.getName(name, MAX_FILENAME_LEN);
  
  raw_data_size = f.available();
  raw_data = new unsigned char[raw_data_size + 1];

  if (!raw_data) {
      log_panic("Failed to allocate raw data buffer for: %s\n", name);
  }
  size_t bytes_read = f.read(raw_data, raw_data_size);
  raw_data[bytes_read] = '\0';
}


Resource::Resource(const Resource& other)
  : res_type(other.res_type), id(other.id), raw_data_size(other.raw_data_size) {
  if (other.name) {
    // Deep copy the name
    for (int i = 0; i < MAX_FILENAME_LEN; ++i) {
      name[i] = other.name[i];
      if (other.name[i] == '\0') break;
    }
  }

  if (other.raw_data && raw_data_size > 0) {
    raw_data = new unsigned char[raw_data_size];
    // Deep copy the raw_data
    for (unsigned i = 0; i < raw_data_size; ++i) {
      raw_data[i] = other.raw_data[i];
    }
  } else {
    raw_data = nullptr;
  }
}


Resource::~Resource() {
  if (raw_data && raw_data_size)
    delete []raw_data;
}


Data_Resource::Data_Resource() : Resource() {}
Data_Resource::Data_Resource(unsigned id, struct File32 &f) : Resource(id, DATA_FIlE, f) {}

const char *Data_Resource::get_text(size_t &len) const {
  len = raw_data_size;
  return (const char *)raw_data;
}


Model *generate_obj_model(void) {
  return nullptr;
}
