#pragma once

#include <string>
#include <memory>
#include <Adafruit_SPIFlash.h>

namespace Device {

struct Resource {
  Resource();
  Resource(int id, File32 &f);
  Resource(const Resource& other); // Copy constructor
  Resource& operator=(const Resource& other); // Copy assignment

  int id;
  unsigned size;
  char *data;
};


class Resource_manager {
  public:
    Resource_manager(void);
    ~Resource_manager();

    int load_resource(const char *);
    Resource *get_resource(int id);
  private:
    static constexpr int MAX_RESOURCES = 16;   // arbitrary

    Adafruit_FlashTransport_QSPI flash_transport;
    Adafruit_SPIFlash flash;
    FatVolume file_sys;

    int res_idx;
    Resource **resources;

    void QSPI_flash_init();
};

} // Namespace Device