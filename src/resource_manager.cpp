#include "resource_manager.h"

#include <Arduino.h>
#include <vector>
#include <cstdint>

#include "util/helpers.h"

#include <vector>
#include <string>
#include <sstream>
#include <iostream>


#pragma pack(push, 1) // ensure proper alignment of bitmap struct in memory
// BMP file header structures
struct bitmap_file_header_t {
  // --- BITMAPFILEHEADER (14 bytes) fields ---
  uint16_t bfType;      // Specifies the file type, must be 0x4D42 ('BM')
  uint32_t bfSize;      // Specifies the size, in bytes, of the bitmap file
  uint16_t bfReserved1; // Reserved; must be 0
  uint16_t bfReserved2; // Reserved; must be 0
  uint32_t bfOffBits;   // Specifies the offset, in bytes, from the BITMAPFILEHEADER
                        // structure to the bitmap bits (which is 14 + 40 = 54 for this case).

  // --- BITMAPINFOHEADER (40 bytes) fields ---
  uint32_t biSize;          // Specifies the number of bytes required by the structure.
                            // For this info header part, it's 40.
  int32_t  biWidth;         // Specifies the width of the bitmap, in pixels.
  int32_t  biHeight;        // Specifies the height of the bitmap, in pixels.
                            // Positive for bottom-up, negative for top-down DIB.
  uint16_t biPlanes;        // Specifies the number of planes for the target device. Must be 1.
  uint16_t biBitCount;      // Specifies the number of bits per pixel.
                            // For 24-bit BMPs, this will be 24.
  uint32_t biCompression;   // Specifies the type of compression for a compressed bitmap.
                            // 0 for BI_RGB (no compression).
  uint32_t biSizeImage;     // Specifies the size, in bytes, of the image.
                            // Can be 0 for BI_RGB bitmaps.
  int32_t  biXPelsPerMeter; // Specifies the horizontal resolution, in pixels per meter.
  int32_t  biYPelsPerMeter; // Specifies the vertical resolution, in pixels per meter.
  uint32_t biClrUsed;       // Specifies the number of color indexes in the color table
                            // that are actually used by the bitmap. 0 for 24-bit.
  uint32_t biClrImportant;  // Specifies the number of color indexes required for
                            // displaying the bitmap. 0 if all colors are important.
};
#pragma pack(pop)

using std::vector;


/**
 * Decodes vertex and UV data from OBJ file content, populating a Model struct.
 * Parses 'v' (vertex), 'vt' (texture UV), and 'f' (face) directives.
 * Handles 1-based and negative OBJ indices, and triangulates quad faces.
 *
 * @param obj_file_content The OBJ file content as a char array.
 * @param model The Model struct to populate (assumes `vertices` and `uvs` members).
 * @returns True on success, false otherwise.
 */
 static bool decode_obj_file(const char* content, Model &model) {
    using std::istringstream;
    using std::string;
    using std::vector;

    vector<float3> temp_vertices; // Temporary storage for unique vertex positions

    model.vertices.clear(); // Clear any existing vertex data in the model

    istringstream stream(content);
    string line;
    int line_num = 0; // For more informative error messages

    Serial.printf("--- Starting OBJ decoding ---\n");

    // Read the OBJ file line by line
    while (getline(stream, line)) {
        line_num++;
        // Print the raw line content and its length
        Serial.printf("Line %d (len %d): '%s'\n", line_num, line.length(), line.c_str());

        if (line.empty()) {
            Serial.printf("  Skipping empty line %d.\n", line_num);
            continue; // Skip empty lines
        }

        istringstream iss(line); // Use stringstream to tokenize the line
        string prefix;
        iss >> prefix; // Read the line prefix (e.g., "v", "f")

        Serial.printf("  Prefix: '%s'\n", prefix.c_str());

        if (prefix == "#") { // Skip comment lines
            Serial.printf("  Skipping comment line %d.\n", line_num);
            continue;
        }

        if (prefix == "v") { // Vertex position line (e.g., "v 1.0 2.0 3.0")
            string x_str, y_str, z_str;
            // Check if 3 string components can be extracted after the prefix
            if (iss >> x_str >> y_str >> z_str) {
                Serial.printf("  Extracted v components: x='%s', y='%s', z='%s'\n", x_str.c_str(), y_str.c_str(), z_str.c_str());
                float x, y, z;
                char* endptr_x, *endptr_y, *endptr_z;

                errno = 0; // Clear errno before calls to detect conversion errors
                x = strtof(x_str.c_str(), &endptr_x);
                if (errno == ERANGE || *endptr_x != '\0') {
                    Serial.printf("Error on line %d: Invalid X value '%s' in OBJ file. Endptr char: 0x%X. Errno: %d. Line: '%s'\n", line_num, x_str.c_str(), (unsigned char)*endptr_x, errno, line.c_str());
                    return false;
                }

                errno = 0;
                y = strtof(y_str.c_str(), &endptr_y);
                if (errno == ERANGE || *endptr_y != '\0') {
                    Serial.printf("Error on line %d: Invalid Y value '%s' in OBJ file. Endptr char: 0x%X. Errno: %d. Line: '%s'\n", line_num, y_str.c_str(), (unsigned char)*endptr_y, errno, line.c_str());
                    return false;
                }

                errno = 0;
                z = strtof(z_str.c_str(), &endptr_z);
                if (errno == ERANGE || *endptr_z != '\0') {
                    Serial.printf("Error on line %d: Invalid Z value '%s' in OBJ file. Endptr char: 0x%X. Errno: %d. Line: '%s'\n", line_num, z_str.c_str(), (unsigned char)*endptr_z, errno, line.c_str());
                    return false;
                }
                temp_vertices.emplace_back(x, y, z);
                Serial.printf("  Added vertex: (%.2f, %.2f, %.2f). temp_vertices.size() = %d\n", x, y, z, temp_vertices.size());
            } else {
                Serial.printf("Error on line %d: Not enough components for vertex format. Line: '%s'\n", line_num, line.c_str());
                return false;
            }
        } else if (prefix == "f") { // Face definition line (e.g., "f 1 2 3" or "f 1/1 2/2 3/3")
            Serial.printf("  Processing face line %d.\n", line_num);
            vector<int> face_v_indices; // Stores 1-based vertex indices for the current face

            string vert_component;
            while (iss >> vert_component) {
                size_t slash_pos = vert_component.find('/');
                string v_idx_str = (slash_pos == string::npos) ? vert_component : vert_component.substr(0, slash_pos);

                if (vert_component == "#") // encountered inline comment, move to next line
                  break;

                int v_idx;
                char* endptr_idx;
                errno = 0; // Clear errno before strtol call
                long val = strtol(v_idx_str.c_str(), &endptr_idx, 10);
                v_idx = static_cast<int>(val);

                if (errno == ERANGE || *endptr_idx != '\0' || v_idx_str.empty()) {
                    Serial.printf("Error on line %d: Invalid vertex index format '%s'. Endptr char: 0x%X. Errno: %d. Line: '%s'\n", line_num, v_idx_str.c_str(), (unsigned char)*endptr_idx, errno, line.c_str());
                    return false;
                }
                face_v_indices.push_back(v_idx);
                Serial.printf("    Parsed face component: '%s', index: %d\n", vert_component.c_str(), v_idx);
            }

            if (face_v_indices.size() < 3) {
                Serial.printf("Warning on line %d: Face with less than 3 vertices (%d) skipped: %s\n", line_num, face_v_indices.size(), line.c_str());
                continue;
            }

            Serial.printf("    Face indices (1-based): %d, %d, %d", face_v_indices[0], face_v_indices[1], face_v_indices[2]);
            if (face_v_indices.size() == 4) Serial.printf(", %d", face_v_indices[3]);
            Serial.printf("\n");

            // Get indices (0-based) for the first triangle (v0, v1, v2)
            int idx0 = face_v_indices[0];
            int idx1 = face_v_indices[1];
            int idx2 = face_v_indices[2];

            idx0 = (idx0 < 0) ? temp_vertices.size() + idx0 : idx0 - 1;
            idx1 = (idx1 < 0) ? temp_vertices.size() + idx1 : idx1 - 1;
            idx2 = (idx2 < 0) ? temp_vertices.size() + idx2 : idx2 - 1;

            Serial.printf("    Converted 0-based indices: %d, %d, %d\n", idx0, idx1, idx2);

            if (temp_vertices.empty() || idx0 < 0 || idx0 >= temp_vertices.size() ||
                idx1 < 0 || idx1 >= temp_vertices.size() ||
                idx2 < 0 || idx2 >= temp_vertices.size()) {
                Serial.printf("Error on line %d: Vertex index out of bounds or temp_vertices empty! (%d, %d, %d). temp_vertices size: %d. Line: '%s'\n", 
                              line_num, idx0, idx1, idx2, temp_vertices.size(), line.c_str());
                model.vertices.clear();
                return false;
            }

            model.vertices.push_back(temp_vertices[idx0]);
            model.vertices.push_back(temp_vertices[idx1]);
            model.vertices.push_back(temp_vertices[idx2]);
            Serial.printf("    Added first triangle. model.vertices.size() = %d\n", model.vertices.size());


            if (face_v_indices.size() == 4) {
                int idx3 = face_v_indices[3];
                idx3 = (idx3 < 0) ? temp_vertices.size() + idx3 : idx3 - 1;

                Serial.printf("    Converted 0-based 4th index: %d\n", idx3);

                if (idx3 < 0 || idx3 >= temp_vertices.size()) {
                    Serial.printf("Error on line %d: 4th vertex index (%d) out of bounds. temp_vertices size: %d. Line: '%s'\n", 
                                  line_num, idx3, temp_vertices.size(), line.c_str());
                    model.vertices.clear();
                    return false;
                }

                model.vertices.push_back(temp_vertices[idx0]);
                model.vertices.push_back(temp_vertices[idx2]);
                model.vertices.push_back(temp_vertices[idx3]);
                Serial.printf("    Added second triangle for quad. model.vertices.size() = %d\n", model.vertices.size());
            }
        }
    }

    Serial.printf("--- OBJ decoding finished. Final temp_vertices size: %d, model.vertices size: %d ---\n", temp_vertices.size(), model.vertices.size());
    return true;
}


/**
 * Helper to get a specific tile from bitmap data, assuming the atlas dimensions
 * match the global ATLAS_WIDTH_PX and ATLAS_HEIGHT_PX constants.
 * * @param atlas_data: Pointer to the raw uint16_t pixel data of the entire atlas.
 * @param tile_id: ID of tile within the bitmap (0-indexed).
 * @param output_tile_data: Buffer for the outputted tile. This buffer MUST be pre-allocated to at least TILE_PIXEL_COUNT.
 * @returns true on success, false otherwise.
 */
static bool _get_tile_from_atlas(const uint16_t* atlas_data, unsigned tile_id, uint16_t *output_tile_data) {
  // Check if tile_id is within the valid range based on the defined constants
  if (tile_id >= TOTAL_TILES) { 
    Serial.printf("Error: tile_id %u is out of bounds for total tiles %u\n", tile_id, TOTAL_TILES);
    return false;
  }

  // Calculate top-left pixel coordinates of the desired tile within the atlas.
  int tile_x_in_atlas = (tile_id % TILES_PER_ROW) * TILE_WIDTH_PX;
  int tile_y_in_atlas = (tile_id / TILES_PER_ROW) * TILE_HEIGHT_PX;

  // Final bounds check using the global ATLAS_WIDTH_PX and ATLAS_HEIGHT_PX.
  if (tile_x_in_atlas + TILE_WIDTH_PX > ATLAS_WIDTH_PX ||
      tile_y_in_atlas + TILE_HEIGHT_PX > ATLAS_HEIGHT_PX) {
    Serial.printf("Error: Calculated tile bounds (%d,%d to %d,%d) exceed global atlas dimensions (%d,%d)\n",
                  tile_x_in_atlas, tile_y_in_atlas,
                  tile_x_in_atlas + TILE_WIDTH_PX, tile_y_in_atlas + TILE_HEIGHT_PX,
                  ATLAS_WIDTH_PX, ATLAS_HEIGHT_PX);
    return false;
  }

  // Copy pixels from the atlas to the output buffer
  for (int y = 0; y < TILE_HEIGHT_PX; ++y) {
    for (int x = 0; x < TILE_WIDTH_PX; ++x) {
      // Access atlas_data using global ATLAS_WIDTH_PX for row stride
      int source_idx = (tile_y_in_atlas + y) * ATLAS_WIDTH_PX + (tile_x_in_atlas + x);
      int dest_idx = y * TILE_WIDTH_PX + x;
      output_tile_data[dest_idx] = atlas_data[source_idx];
    }
  }

  return true;
}



/**
 * Initializes resoure pool and initializes on board flash
 */
Resource_manager::Resource_manager() {
  this->num_resources = 0;
  for (int i = 0; i < MAX_RESOURCES; ++i) {
    this->resources[i].data = nullptr;
    this->resources[i].length = 0;
    this->resources[i].type = INVALID_FILE;
  }
}


// dealloc
Resource_manager::~Resource_manager() {
  for (int i = 0; i < MAX_RESOURCES; ++i) {
    this->unload_resource(i);
  }
}


/**
 * Loads a resource into the pool from a given path, after a resource is loaded,
 * it can be retrieved using the id returned by this method.
 * 
 * @param path path to the desired file
 * @returns a valid id >= 0 upon success, INVALID_RESOURCE on failure
 */
resource_id_t Resource_manager::load_resource(const char *path) {
  File32 f = this->file_sys.open(path, O_RDONLY);
  if (!f.available() || f.fileSize() == 0) // is the file valid and readble?
    return INVALID_RESOURCE;
  

  // get filename
  // max filename size on FAT12 is 13 bytes with a null terminator
  const size_t file_name_size = 14;
  char file_name[file_name_size];
  Serial.printf("getting file name\n");
  f.getName7(file_name, file_name_size); // get name as ascii
  if (!file_name)
    return INVALID_RESOURCE;
  
  Serial.printf("Loading %s...\n", file_name);

  char *extension = strchr(file_name, '.');
  
  // Are we reading a bitmap file?
  // bitmap files are binary and have to be handled differently than text files 
  if (strcmp(extension, ".bmp") == 0) {
    Serial.println("Loading bitmap data");
    
    if (this->load_bmp(f)) {
      f.close();
      return this->num_resources++; // bitmap loaded successfully. close file object, return current number of resources, then increment
    } else {
      Serial.println("Failed to load file");
    }
  } else {
    Serial.println("Loading text data");

    if (this->load_text(f)) {
      f.close();
      return this->num_resources++; // text file loaded successfully. close file object, return current number of resources, then increment
    } else {
      Serial.println("Failed to load file");
    }
  }
  
  f.close();
  return INVALID_RESOURCE;
}


/**
 * Get a pointer to a resource in the pool by its id. Resource may not be valid!!
 * @param id id of the resource to get
 * @returns A const pointer to the resource, nullptr upon id out of bounds 
 */
const Resource_entry_t *Resource_manager::get_resource(resource_id_t id) {
  if (id >= 0 && id < MAX_RESOURCES)
    return (const Resource_entry_t*)&this->resources[id];

  Serial.printf("Resource Index out of bounds! id: %d\n", id);
  return nullptr;
}


// FIXME ! Doesn't allow a new resource to be allocated in an old ones spot
//         Rearrangine the internal array would break the logic for rettrieving 
//         Resources. All it does is ensure memory deletion and cleans up the entry
bool Resource_manager::unload_resource(resource_id_t id) {
  if (id >= 0 && id < MAX_RESOURCES) {
    Resource_entry_t entry = this->resources[id];

    if (entry.data) {
      if (entry.type == BITMAP_FILE) {
        delete []reinterpret_cast<uint16_t*>(entry.data);
      } else if (entry.type == DATA_FIlE) {
        delete []reinterpret_cast<char*>(entry.data);
      }

      entry.data = nullptr;
      entry.type = INVALID_FILE;
      entry.length = 0;
    }
  }

  return false;
}


/**
 *  Read a data file from the resource pool, populating the passed model struct
 *  With vertex data.
 *  @param id: ID of the desired resource, must be populated
 *  @param model: The model to be populated
 *  @returns true on success, false otherwise
*/
bool Resource_manager::read_obj_resource(resource_id_t id, Model &model) {
  if (id >= 0 && id < MAX_RESOURCES) {
    Resource_entry_t res = this->resources[id];

    if (res.type == DATA_FIlE) {
      if (res.length > 0) {
        return decode_obj_file(reinterpret_cast<const char *>(res.data), model);
      }

      Serial.printf("Resource at %d has size %u\n", id, this->resources[id].length);
    }

    Serial.printf("Resource at %d is not a DATA_FILE\n", id);
  } else {
    Serial.printf("id: %d out of bounds.\n", id);
  }


  return false;
}


/**
 * Gets a specific tile index from the passed resource.
 * Tile 0 is at top-left corner of the bitmap
 * 
 * @param id: ID of the bitmap in the resource pool
 * @param tile_id: ID of tile within the bitmap
 * @param output_tile_data: Buffer for the outputted tile, This 
 *                          buffer MUST be pre-allocated to attleast TILE_PIXEL_COUNT
 * @returns true on success, false otherwise
 */
bool Resource_manager::get_tile_from_atlas(resource_id_t id, unsigned tile_id, uint16_t *output_tile_data) {
  if (id >= 0 && id < MAX_RESOURCES) {
    Resource_entry_t res = this->resources[id];
    
    if (res.type == BITMAP_FILE) {
      if (res.length > 0) {
        return _get_tile_from_atlas(reinterpret_cast<const uint16_t*>(res.data), tile_id, output_tile_data);
      }

      Serial.printf("Resource at %d has size %u\n", id, this->resources[id].length);
    } else {
      Serial.printf("Resource at %d is not a BITMAP_FILE\n", id);
    }
  } else {
    Serial.printf("id: %d out of bounds.\n", id);
  }

  return false;
}


/**
 * Initialize onboard QSPI FLash
 */
void Resource_manager::init_QSPI_flash() {
  this->flash = Adafruit_SPIFlash(&this->flash_transport);
  this->file_sys = FatVolume();

  if (this->flash.begin()) {
    log("Filesystem found\n");
    log("Flash chip JEDEC ID: %#04x\n", this->flash.getJEDECID());
    log("Flash chip size: %lu bytes\n", this->flash.size());

    // First call begin to mount the filesystem.  Check that it returns true
    // to make sure the filesystem was mounted.
    if (!this->file_sys.begin(&this->flash)) {
      log_panic("Failed to mount Filesystem.\n");
    }

    log("Dumping root directory\n");
    this->file_sys.dmpRootDir(&Serial);
    log("Filesystem mounted successfully.\n");
  }
}


/**
 * Read in a text file from flash, store in resource pool
 * @param f Open file to read from, assumes f is valid and readable
 * @returns true upon successful read, flase otherwise
 */
bool Resource_manager::load_text(File32 &f) {
  Resource_entry_t *entry = &this->resources[this->num_resources];
  if (!entry) {
    return false;
  }

  entry->type = DATA_FIlE;
  
  // get filesize
  f.seekEnd();
  entry->length = f.curPosition() + 1;
  f.rewind();

  // allocate buffer
  Serial.printf("Allocated %u bytes for resource\n", entry->length);
  entry->data = new char[entry->length];
  if (!entry->data)
    log_panic("Failed to allocated %u bytes for file load\n", entry->length);

  // read file into memory, stop when an EOF character is encountered
  int16_t c, idx = 0;
  while ((c = f.read()) != EOF) {
    uint8_t byte = static_cast<uint8_t>(c & 0xFF);
    static_cast<char*>(entry->data)[idx++] = (char)byte;
  }

  static_cast<char*>(entry->data)[idx] = '\0'; // ensure null termination
  return true;
}


/**
 * Read in a bitmap from flash, store the pixel data in rgb 565 format, ready to be blitted to the display.
 * @param f Open file to read from, assumes f is valid and readable
 * @returns true upon successful decode, false otherwise
 */
bool Resource_manager::load_bmp(File32 &f) {
  bitmap_file_header_t header;
  Resource_entry_t *entry = &this->resources[this->num_resources];
  if (!entry) {
    return false;
  }

  size_t read = f.read(&header, sizeof(bitmap_file_header_t));
  if (read != sizeof(bitmap_file_header_t)) {
    log_panic("Expected to read %u bytes but only read %u bytes\n", sizeof(bitmap_file_header_t), read);
  }

  if (header.bfType != 0x4D42) { // 0x4D42 is 'BM' in little-endian
    log_panic("Error: Not a valid BMP file (bfType is not 'BM').");
    f.close();
    return false;
  }


  Serial.printf("--- BMP File Header ---\n");
  Serial.printf("bfType (should be 0z4D42): 0x%hX\n", header.bfType); // %hX for unsigned short in hex
  Serial.printf("bfSize (total file size): %u bytes\n", header.bfSize); // %u for unsigned int
  Serial.printf("bfOffBits (pixel data offset): %u bytes\n", header.bfOffBits); // %u for unsigned int
  Serial.printf("\n--- BMP Info Header ---\n");
  Serial.printf("biSize (info header size): %u bytes\n", header.biSize);
  Serial.printf("biWidth: %d pixels\n", header.biWidth); // %d for signed int
  Serial.printf("biHeight: %d pixels\n", header.biHeight); // %d for signed int
  Serial.printf("biPlanes: %hu\n", header.biPlanes); // %hu for unsigned short
  Serial.printf("biBitCount (bits per pixel): %hu\n", header.biBitCount); // %hu for unsigned short
  Serial.printf("biCompression (0 for BI_RGB): %u\n", header.biCompression);
  Serial.printf("biSizeImage (image data size): %u bytes\n", header.biSizeImage);
  Serial.printf("biXPelsPerMeter: %d\n", header.biXPelsPerMeter);
  Serial.printf("biYPelsPerMeter: %d\n", header.biYPelsPerMeter);
  Serial.printf("biClrUsed: %u\n", header.biClrUsed);
  Serial.printf("biClrImportant: %u\n", header.biClrImportant);

  // Validate essential info for a 24-bit BMP
  if (header.biSize != 40)
    log_panic("Warning: biSize is not 40. This might not be a standard Windows BMP info header.");
  if (header.biBitCount != 32)
    log_panic("Error: This is not a 32-bit BMP (biBitCount is %u).\n", (unsigned int)header.biBitCount);
  if (header.biCompression != 0) // BI_RGB
    log_panic("Error: Only uncompressed (BI_RGB) 24-bit BMPs are supported (biCompression is %u).\n", header.biCompression);

  // Determine actual image height (absolute value)
  int32_t imageHeight = abs(header.biHeight);

  // Store dimensions in the resource entry
  entry->width = header.biWidth;
  entry->height = imageHeight;

  entry->length = header.biWidth * imageHeight;
  entry->data = new uint16_t[entry->length];
  if (!entry->data)
    log_panic("Failed to allocate buffer for resource of size %u", entry->length);
  
  // Calculate row size in bytes dynamically based on biBitCount
  // Rows in BMP are padded to be a multiple of 4 bytes.
  int bytes_per_pixel = header.biBitCount / 8; // Calculate bytes per pixel
  int rowStride = header.biWidth * bytes_per_pixel; // Bytes per row without padding
  int padding = (4 - (rowStride % 4)) % 4; // Padding bytes for each row

  Serial.printf("bytes_per_pixel: %d\n", bytes_per_pixel);
  Serial.printf("rowPadding: %d\n", padding);

  // Jump to start of pixel data
  if (!f.seek(header.bfOffBits)) {
    Serial.printf("Failed to seek to pixel data, offset=0x%X\n", header.bfOffBits);
    return false;
  }
  
  // Determine if the BMP is bottom-up (positive biHeight) or top-down (negative biHeight)
  bool isBottomUp = (header.biHeight > 0);

  for (int y_file = 0; y_file < imageHeight; ++y_file) { // y_file iterates through rows as they appear in the file
    // Calculate the target y-coordinate in our memory buffer
    // If bottom-up, we read the first row from file (y_file=0) into the last row in memory (imageHeight-1).
    // If top-down, we read the first row from file (y_file=0) into the first row in memory (0).
    int y_mem = isBottomUp ? (imageHeight - 1 - y_file) : y_file;

    for (int x = 0; x < header.biWidth; ++x) {
      uint8_t b, g, r, a;

      // read in bgr values
      size_t current_read_bytes = 0;
      current_read_bytes += f.read(reinterpret_cast<char*>(&b), 1);
      current_read_bytes += f.read(reinterpret_cast<char*>(&g), 1);
      current_read_bytes += f.read(reinterpret_cast<char*>(&r), 1);

      if (bytes_per_pixel == 4) { // 32 bit Bitmap has alpha channel, we read it but ignore the value for RGB565
        current_read_bytes += f.read(reinterpret_cast<char*>(&a), 1);
      }

      if (current_read_bytes != bytes_per_pixel) {
        Serial.println("Failed to read in BGR values");
        return false;
      }

      // Convert BGR to 16-bit RGB 565 format
      // RRRRRGGG GGGBBBBB
      // Red: 5 bits (MSB) from original 8 bits
      // Green: 6 bits (Middle) from original 8 bits
      // Blue: 5 bits (LSB) from original 8 bits
      uint16_t rgb565_pixel = ((r & 0xF8) << 8) |
                              ((g & 0xFC) << 3) |
                              ((b & 0xF8) >> 3);
      
      // Store the pixel in the calculated memory location
      reinterpret_cast<uint16_t *>(entry->data)[x + header.biWidth * y_mem] = rgb565_pixel;
    }

    if (padding > 0) {
      if (!f.seekCur(padding)) {
        Serial.println("Error: Failed to skip padding bytes");
        return false;
      }
    }
  }

  entry->type = BITMAP_FILE;
  return true;
}

