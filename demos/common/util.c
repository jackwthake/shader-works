#include "util.h"

#include <stdio.h>
#include <stdlib.h>

bool load_bmp_texture(const char *filename, u32 *out_width, u32 *out_height, u32 **out_data) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    fprintf(stderr, "Failed to open BMP file: %s\n", filename);
    return false;
  }

  // Read BMP header
  u16 signature;
  fread(&signature, sizeof(u16), 1, file);
  if (signature != 0x4D42) { // 'BM' in little-endian
    fprintf(stderr, "Invalid BMP file: %s\n", filename);
    fclose(file);
    return false;
  }

  fseek(file, 18, SEEK_SET); // Move to width/height in header
  u32 width, height;
  fread(&width, sizeof(u32), 1, file);
  fread(&height, sizeof(u32), 1, file);

  fseek(file, 54, SEEK_SET); // Move to pixel data
  u32 row_padded = (width * 3 + 3) & (~3); // Each row is padded to a multiple of 4 bytes
  u8 *data = malloc(row_padded * height);
  if (!data) {
    fprintf(stderr, "Failed to allocate memory for BMP data\n");
    fclose(file);
    return false;
  }

  fread(data, sizeof(u8), row_padded * height, file);
  fclose(file);

  // Convert BGR to RGBA
  u32 *rgba_data = malloc(width * height * sizeof(u32));
  if (!rgba_data) {
    fprintf(stderr, "Failed to allocate memory for RGBA data\n");
    free(data);
    return false;
  }

  for (u32 y = 0; y < height; y++) {
    for (u32 x = 0; x < width; x++) {
      u8 b = data[y * row_padded + x * 3 + 0];
      u8 g = data[y * row_padded + x * 3 + 1];
      u8 r = data[y * row_padded + x * 3 + 2];
      rgba_data[(height - y - 1) * width + x] = (r << 24) | (g << 16) | (b << 8) | 0xFF; // RGBA with full alpha
    }
  }

  free(data);

  *out_width = width;
  *out_height = height;
  *out_data = rgba_data;
}
