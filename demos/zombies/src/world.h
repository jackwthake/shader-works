#ifndef __WORLD_H__
#define __WORLD_H__

#include <shader-works/renderer.h>

#define MAX_LIGHTS 5
#define MAX_ENTITIES 5

typedef enum {
  DIR_NORTH,
  DIR_SOUTH,
  DIR_EAST,
  DIR_WEST
} direction_t;

typedef struct sector_t {
  int x, z, w, d, ceiling_height, floor_height;
  model_t *walls;
  usize num_walls;

  model_t floor, ceiling;
  struct sector_t *next, *prev;
  bool visited; // For portal rendering
} sector_t;

typedef struct {
  transform_t transform;
  model_t mesh;
  sector_t *current_sector;
} entity_t;

typedef struct {
  sector_t *sectors, *tail;
  light_t *lights;
  entity_t *entities;

  usize num_sectors, num_lights, num_entities;
} world_t;

void init_world(world_t *world);
void delete_world(world_t *world);

sector_t* find_neighbor(world_t *world, sector_t *self, direction_t side);

void generate_random_map(world_t *world, int room_count);
void finalize_world_geometry(world_t *world);

void tick_entities(world_t *world, f32 delta_time);
void render_world(renderer_t *renderer, world_t *world, transform_t *camera);

#endif // __WORLD_H__
