#ifndef CONFIG_H
#define CONFIG_H

#include <cJSON.h>
#include <stddef.h>

// Global config object
extern cJSON *g_config;

// World configuration (loaded from config.json)
typedef struct {
  int seed;
  int chunk_size;
  int half_chunk_size;
  int ground_segments_per_chunk;
  float ground_segment_size;
  int chunk_load_radius;
  int max_chunks;
} world_config_t;

extern world_config_t g_world_config;

// Load config.json and parse window parameters
// Returns 0 on success, -1 on failure
int load_config(unsigned int *width, unsigned int *height, unsigned int *scale, char *title, size_t title_size);

// Load world configuration from config.json
// Must be called after load_config()
// Returns 0 on success, -1 on failure
int load_world_config(void);

// Free the global config object
void free_config(void);

#endif // CONFIG_H
