#pragma once

#include <stdint.h>
#include <stddef.h>

enum RESOURCE_TYPE {
  BITMAP_FILE,
  DATA_FIlE,
  INVALID_FILE
};


class Resource {
  protected:
    Resource();
    Resource(unsigned id, RESOURCE_TYPE type, struct File32 &f);
    ~Resource();

    unsigned char *raw_data;
    size_t raw_data_size;
  
  public:
    Resource(const Resource& other); // Copy constructor

    constexpr static unsigned MAX_FILENAME_LEN = 13;
    const RESOURCE_TYPE res_type;
    const unsigned id;
    char name[MAX_FILENAME_LEN];
};


class Data_Resource : public Resource {
  public:
    Data_Resource();
    Data_Resource(unsigned id, struct File32 &f);

    const char *get_text(size_t &len) const;
    class Model *generate_obj_model(void);
};


class Bitmap_Resource : public Resource {
  void read_bitmap_data();

  public:
    Bitmap_Resource();
    Bitmap_Resource(unsigned id, struct File32 &f);
    Bitmap_Resource(const Bitmap_Resource& other); // Copy constructor

    unsigned width, height;
    uint16_t *pixels;
};