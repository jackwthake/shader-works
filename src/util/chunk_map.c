#include "chunk_map.h"

#include <stdlib.h>

static inline usize get_chunk_hash(int x, int z, usize table_size) {
  return ((x * 73856093) ^ (z * 19349663)) % table_size;
}

static void free_chunk_node(chunk_map_node_t *node) {
  if (!node) return;

  delete_model(&node->chunk.ground_plane);

  for (usize i = 0; i < node->chunk.num_trees; ++i) {
    delete_model(&node->chunk.trees[i]);
  }

  // Free the trees array itself
  if (node->chunk.trees) {
    free(node->chunk.trees);
  }

  free(node);
}

void init_chunk_map(chunk_map_t *map, usize num_buckets) {
  if (!map) return;

  map->num_buckets = num_buckets;
  map->buckets = calloc(num_buckets, sizeof(chunk_map_node_t *));

  map->num_loaded_chunks = 0;
}

void free_chunk_map(chunk_map_t *map) {
  if (!map) return;

  for (usize i = 0; i < map->num_buckets; ++i) {
    chunk_map_node_t *head = map->buckets[i];

    while (head) {
      chunk_map_node_t *next = head->next;
      free_chunk_node(head);

      head = next;
    }
  }

  free(map->buckets);
  map->buckets = NULL;
  map->num_loaded_chunks = 0;
}

void insert_chunk(chunk_map_t *map, chunk_t *chunk) {
  if (!map || !chunk) return;

  usize index = get_chunk_hash(chunk->x, chunk->z, map->num_buckets);

  chunk_map_node_t *old_head = map->buckets[index];

  // emplace new chunk at start of list, no reason to iterate to the end to add
  chunk_map_node_t *head = malloc(sizeof(chunk_map_node_t));
  head->chunk = *chunk;
  head->loaded = true;
  head->next = old_head;

  map->buckets[index] = head;
  ++map->num_loaded_chunks;
}

void remove_chunk(chunk_map_t *map, int x, int z) {
  if (!map) return;

  usize index = get_chunk_hash(x, z, map->num_buckets);
  chunk_map_node_t *head = map->buckets[index], *prev = NULL;

  while (head) {
    if (head->chunk.x == x && head->chunk.z == z) {
      if (prev == NULL) {
        map->buckets[index] = head->next;
      } else {
        prev->next = head->next;
      }

      free_chunk_node(head);
      --map->num_loaded_chunks;
      return;
    }
    
    prev = head;
    head = head->next;
  }
}

void remove_chunk_if(chunk_map_t *map, query_func func, void *param, usize num_params) {
  if (!map) return;

  for (usize i = 0; i < map->num_buckets; ++i) {
    chunk_map_node_t *head = map->buckets[i], *prev = NULL;

    while (head) {
      chunk_map_node_t *next = head->next; // Store next before potential free

      if (func(&head->chunk, param, num_params)) {
        if (prev == NULL) {
          map->buckets[i] = next;
        } else {
          prev->next = next;
        }

        free_chunk_node(head);
        --map->num_loaded_chunks;

        // Continue with next node instead of returning
        head = next;
      } else {
        prev = head;
        head = next;
      }
    }
  }
}

chunk_map_node_t *chunk_lookup(chunk_map_t *map, int x, int z) {
  if (!map) return NULL;

  usize index = get_chunk_hash(x, z, map->num_buckets);
  chunk_map_node_t *head = map->buckets[index];

  while (head) {
    if (head->chunk.x == x && head->chunk.z == z)
      return head;
    
    head = head->next;
  }

  return NULL;
}

bool is_chunk_loaded(chunk_map_t *map, int x, int z) {
  chunk_map_node_t *node = chunk_lookup(map, x, z);
  return node ? node->loaded : false;
}

// used to get all chunks using query_chunk_map
static inline bool always_true(chunk_t *chunk, void *params, usize num_params) {
  (void)chunk; (void)params; (void)num_params;
  return true;
}

void get_all_chunks(chunk_map_t *map, chunk_t **chunk_buf, usize *count) {
  query_chunk_map(map, chunk_buf, count, always_true);
}

void query_chunk_map(chunk_map_t *map, chunk_t **chunk_buf, usize *count, query_func func) {
  if (!map || !chunk_buf || !count) return;

  *count = 0;
  for (usize i = 0; i < map->num_buckets; ++i) {
    chunk_map_node_t *node = map->buckets[i];
    while (node != NULL) {
      if (node->loaded && func(&node->chunk, NULL, 0)) {
        chunk_buf[*count] = &node->chunk;
        (*count)++;
      }
      node = node->next;
    }
  }
}