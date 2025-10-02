#ifndef __CHUNK_MAP_H__
#define __CHUNK_MAP_H__

#include <shader-works/primitives.h>

#define CHUNK_MAP_NUM_BUCKETS 9

typedef struct {
  int x, z;
  model_t ground_plane;
  model_t *trees;
  usize num_trees;
} chunk_t;

typedef struct chunk_map_node_t {
  chunk_t chunk;
  struct chunk_map_node_t *next;
  bool loaded;
} chunk_map_node_t;

typedef struct {
  chunk_map_node_t **buckets;
  usize num_buckets, num_loaded_chunks;
} chunk_map_t;

// return true to include chunk in final chunk buffer
typedef bool (*query_func)(chunk_t *chunk, void *param, usize num_params);

void init_chunk_map(chunk_map_t *map, usize num_buckets);
void free_chunk_map(chunk_map_t *map);

void insert_chunk(chunk_map_t *map, chunk_t *chunk);
void remove_chunk(chunk_map_t *map, int x, int z);
void remove_chunk_if(chunk_map_t *map, query_func, void *param, usize num_params);

chunk_map_node_t *chunk_lookup(chunk_map_t *map, int x, int z);
bool is_chunk_loaded(chunk_map_t *map, int x, int z);

void get_all_chunks(chunk_map_t *map, chunk_t **chunk_buf, usize *count);
void query_chunk_map(chunk_map_t *map, chunk_t **chunk_buf, usize *count, query_func func);

#endif