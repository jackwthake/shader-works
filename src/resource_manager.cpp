#include "resource_manager.h"

#include <Arduino.h>

#include <Adafruit_SPIFlash.h>
#include <SdFat_Adafruit_Fork.h>

#include "renderer.h"
#include "util/helpers.h"


// BMP file header structures
struct BMPHeader {
  uint16_t bfType;
  uint32_t bfSize;
  uint16_t bfReserved1;
  uint16_t bfReserved2;
  uint32_t bfOffBits;
} __attribute__((packed));


struct BMPInfoHeader {
  uint32_t biSize;
  int32_t  biWidth;
  int32_t  biHeight;
  uint16_t biPlanes;
  uint16_t biBitCount;
  uint32_t biCompression;
  uint32_t biSizeImage;
  int32_t  biXPelsPerMeter;
  int32_t  biYPelsPerMeter;
  uint32_t biClrUsed;
  uint32_t biClrImportant;
} __attribute__((packed));


Resource::Resource() : res_type(INVALID_FILE), id(-1) {
  raw_data_size = 0;
  raw_data = nullptr;
}


Resource::Resource(unsigned id, RESOURCE_TYPE type, File32 &f) : id(id), res_type(type) {
  // Read file name safely and ensure null termination
  f.getName(name, MAX_FILENAME_LEN - 1);
  name[MAX_FILENAME_LEN - 1] = '\0';

  // Read file data incrementally in chunks
  raw_data_size = f.size();
  if (raw_data_size == 0 || raw_data_size > 1024 * 1024) { // 1MB safety limit
      log_panic("File too large or empty: %s\n", name);
  }
  raw_data = new unsigned char[raw_data_size + 1];
  if (!raw_data) {
      log_panic("Failed to allocate raw data buffer for: %s\n", name);
  }

  f.seek(0); // Ensure reading from start
  size_t total_read = 0;
  const size_t chunk_size = 256;
  while (total_read < raw_data_size) {
      size_t to_read = (raw_data_size - total_read > chunk_size) ? chunk_size : (raw_data_size - total_read);
      Serial.printf("%s: Total bytes read: %u of %u bytes. Attempting to read %u bytes\n", name, total_read, raw_data_size, to_read);
      Serial.flush();
      size_t bytes_read = f.read(raw_data + total_read, to_read);
      if (bytes_read == 0) {
          log_panic("Failed to read file data for: %s\n", name);
      }
      total_read += bytes_read;
  }

  raw_data[raw_data_size] = '\0'; // Null-terminate for safety
}


Resource::Resource(const Resource& other) : res_type(other.res_type), id(other.id), raw_data_size(other.raw_data_size) {
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


Bitmap_Resource::Bitmap_Resource() : Resource() {
  width = 0;
  height = 0;
  pixels = nullptr;
}


Bitmap_Resource::Bitmap_Resource(unsigned id, struct File32 &f) : Resource(id, BITMAP_FILE, f) {
  this->read_bitmap_data();
}


Bitmap_Resource::Bitmap_Resource(const Bitmap_Resource& other) : Resource(other), width(other.width), height(other.height) {
  if (other.pixels && width > 0 && height > 0) {
    pixels = new uint16_t[width * height];
    for (int i = 0; i < width * height; ++i) {
      pixels[i] = other.pixels[i];
    }
  } else {
    pixels = nullptr;
  }
}


void Bitmap_Resource::read_bitmap_data() {
  // Safety: Ensure we have enough data for headers
  if (!raw_data || raw_data_size < sizeof(BMPHeader) + sizeof(BMPInfoHeader)) {
    width = height = 0;
    pixels = nullptr;
    return;
  }

  const BMPHeader* hdr = reinterpret_cast<const BMPHeader*>(raw_data);
  if (hdr->bfType != 0x4D42) { // 'BM' signature
    width = height = 0;
    pixels = nullptr;
    return;
  }

  const BMPInfoHeader* info = reinterpret_cast<const BMPInfoHeader*>(raw_data + sizeof(BMPHeader));
  if (info->biBitCount != 24 && info->biBitCount != 32) {
    width = height = 0;
    pixels = nullptr;
    return;
  }
  if (info->biCompression != 0) { // Only support uncompressed
    width = height = 0;
    pixels = nullptr;
    return;
  }

  Serial.println("Header loaded");

  width = info->biWidth;
  height = (info->biHeight > 0) ? info->biHeight : -info->biHeight;
  bool flip = info->biHeight > 0; // BMP is bottom-up if height > 0

  Serial.printf("bmp is %u x %u \n", width, height);

  // Calculate row size (padded to 4 bytes)
  int bytes_per_pixel = info->biBitCount / 8;
  int row_stride = ((width * bytes_per_pixel + 3) / 4) * 4;
  size_t pixel_data_offset = hdr->bfOffBits;
  if (raw_data_size < pixel_data_offset + row_stride * height) {
    width = height = 0;
    pixels = nullptr;
    return;
  }

  // Free previous pixels if any
  if (pixels) {
    delete[] pixels;
    pixels = nullptr;
  }
  pixels = new uint16_t[width * height];

  for (int y = 0; y < height; ++y) {
    int src_y = flip ? (height - 1 - y) : y;
    Serial.println(y);
    const unsigned char* row = raw_data + pixel_data_offset + src_y * row_stride;
    for (int x = 0; x < width; ++x) {
      int idx = y * width + x;
      Serial.println(idx);
      uint8_t b = row[x * bytes_per_pixel + 0];
      uint8_t g = row[x * bytes_per_pixel + 1];
      uint8_t r = row[x * bytes_per_pixel + 2];
      // Convert to RGB565
      pixels[idx] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
  }
}