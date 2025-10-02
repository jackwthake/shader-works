#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global config object
cJSON *g_config = NULL;

// Global world config
world_config_t g_world_config = {0};

// Default values
#define DEFAULT_TITLE "Tundra"
#define DEFAULT_WIDTH 200
#define DEFAULT_HEIGHT 125
#define DEFAULT_SCALE 8

// Default world values
#define DEFAULT_WORLD_SEED 2
#define DEFAULT_CHUNK_SIZE 32
#define DEFAULT_GROUND_SEGMENTS_PER_CHUNK 4
#define DEFAULT_CHUNK_LOAD_RADIUS 1

int load_config(unsigned int *width, unsigned int *height, unsigned int *scale, char *title, size_t title_size) {
  FILE *config_file = fopen("config.json", "r");

  // Set defaults
  *width = DEFAULT_WIDTH;
  *height = DEFAULT_HEIGHT;
  *scale = DEFAULT_SCALE;
  strncpy(title, DEFAULT_TITLE, title_size - 1);
  title[title_size - 1] = '\0';

  if (!config_file) {
    printf("config.json not found, using defaults: %s [%ux%u @ scale %u]\n",
           title, *width, *height, *scale);
    return -1;
  }

  // Read file
  fseek(config_file, 0, SEEK_END);
  long file_size = ftell(config_file);
  fseek(config_file, 0, SEEK_SET);

  char *json_data = (char *)malloc(file_size + 1);
  if (!json_data) {
    fclose(config_file);
    printf("Failed to allocate memory for config.json\n");
    return -1;
  }

  fread(json_data, 1, file_size, config_file);
  json_data[file_size] = '\0';
  fclose(config_file);

  // Parse JSON
  g_config = cJSON_Parse(json_data);
  free(json_data);

  if (!g_config) {
    printf("Failed to parse config.json, using defaults\n");
    return -1;
  }

  // Extract window settings
  cJSON *window = cJSON_GetObjectItem(g_config, "window");
  if (window) {
    cJSON *json_title = cJSON_GetObjectItem(window, "title");
    cJSON *json_width = cJSON_GetObjectItem(window, "width");
    cJSON *json_height = cJSON_GetObjectItem(window, "height");
    cJSON *json_scale = cJSON_GetObjectItem(window, "scale");

    if (cJSON_IsString(json_title)) {
      strncpy(title, json_title->valuestring, title_size - 1);
      title[title_size - 1] = '\0';
    }
    if (cJSON_IsNumber(json_width)) *width = (unsigned int)json_width->valueint;
    if (cJSON_IsNumber(json_height)) *height = (unsigned int)json_height->valueint;
    if (cJSON_IsNumber(json_scale)) *scale = (unsigned int)json_scale->valueint;

    printf("Loaded window config: %s [%ux%u @ scale %u]\n",
           title, *width, *height, *scale);
  }

  return 0;
}

int load_world_config(void) {
  // Set defaults
  g_world_config.seed = DEFAULT_WORLD_SEED;
  g_world_config.chunk_size = DEFAULT_CHUNK_SIZE;
  g_world_config.ground_segments_per_chunk = DEFAULT_GROUND_SEGMENTS_PER_CHUNK;
  g_world_config.chunk_load_radius = DEFAULT_CHUNK_LOAD_RADIUS;

  if (!g_config) {
    printf("Config not loaded, using default world settings\n");
    goto calculate_derived;
  }

  // Extract world settings
  cJSON *world = cJSON_GetObjectItem(g_config, "world");
  if (world) {
    cJSON *seed = cJSON_GetObjectItem(world, "seed");
    cJSON *chunk_size = cJSON_GetObjectItem(world, "chunk_size");
    cJSON *ground_segments = cJSON_GetObjectItem(world, "ground_segments_per_chunk");
    cJSON *load_radius = cJSON_GetObjectItem(world, "chunk_load_radius");

    if (cJSON_IsNumber(seed)) g_world_config.seed = seed->valueint;
    if (cJSON_IsNumber(chunk_size)) g_world_config.chunk_size = chunk_size->valueint;
    if (cJSON_IsNumber(ground_segments)) g_world_config.ground_segments_per_chunk = ground_segments->valueint;
    if (cJSON_IsNumber(load_radius)) g_world_config.chunk_load_radius = load_radius->valueint;

    printf("Loaded world config: seed=%d, chunk_size=%d, segments=%d, load_radius=%d\n",
           g_world_config.seed, g_world_config.chunk_size,
           g_world_config.ground_segments_per_chunk, g_world_config.chunk_load_radius);
  }

calculate_derived:
  // Calculate derived values
  g_world_config.half_chunk_size = g_world_config.chunk_size / 2;
  g_world_config.ground_segment_size = (float)g_world_config.chunk_size / (float)g_world_config.ground_segments_per_chunk;
  g_world_config.max_chunks = (g_world_config.chunk_load_radius * 2 + 1) * (g_world_config.chunk_load_radius * 2 + 1);

  return 0;
}

void free_config(void) {
  if (g_config) {
    cJSON_Delete(g_config);
    g_config = NULL;
  }
}
