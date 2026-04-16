#ifndef __WORLD_H__
#define __WORLD_H__

#include <shader-works/renderer.h>

#define MAX_LIGHTS 10

typedef struct sector_t {
  int x, y, w, d, ceiling_height;
  model_t *walls;
  usize num_walls;

  model_t floor, ceiling;
  struct sector_t *next, *prev;
} sector_t;

typedef struct {
  sector_t *sectors, *tail;
  usize num_sectors;

  light_t *lights;
  usize num_lights;
} world_t;

void init_world(world_t *world);
void delete_world(world_t *world);

void add_sector(world_t *world, int x, int y, int w, int d, int ceiling_height);
void add_walls(world_t *world);
void render_world(renderer_t *renderer, world_t *world, transform_t *camera);

#endif // __WORLD_H__
