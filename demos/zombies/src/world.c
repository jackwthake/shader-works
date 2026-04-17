#include "world.h"

#include <stdlib.h>
#include <math.h>

#include <SDL3/SDL.h>
#include <shader-works/maths.h>

#include "world.h"

extern fragment_shader_t ground_frag;

void init_world(world_t *world) {
  world->sectors = NULL;
  world->tail = NULL;
  world->num_sectors = 0;
  world->lights = calloc(MAX_LIGHTS, sizeof(light_t));
  world->num_lights = 0;
  world->entities = calloc(MAX_ENTITIES, sizeof(entity_t));
  world->num_entities = 0;
}

void delete_world(world_t *world) {
  sector_t *sector = world->sectors;
  while (sector) {
    sector_t *next = sector->next;
    free(sector);
    sector = next;
  }

  world->sectors = NULL;
  world->tail = NULL;
  world->num_sectors = 0;

  if (world->lights) {
    free(world->lights);
    world->num_lights = 0;
  }

  if (world->entities) {
    free(world->entities);
    world->num_entities = 0;
  }
}

static sector_t *add_sector(world_t *world, int x, int z, int w, int d, int ceiling_height, int floor_height) {
  // Check if a sector already exists at this exact X, Z
  sector_t *check = world->sectors;
  while (check) {
      if (check->x == x && check->z == z) return NULL;
      check = check->next;
  }

  sector_t *new_sector = malloc(sizeof(sector_t));
  new_sector->x = x;
  new_sector->z = z;
  new_sector->w = w;
  new_sector->d = d;
  new_sector->ceiling_height = ceiling_height;
  new_sector->floor_height = floor_height;
  new_sector->next = NULL;
  new_sector->prev = world->tail;
  new_sector->num_walls = 0;
  new_sector->walls = NULL;
  new_sector->visited = false;

  new_sector->floor = (model_t){0};
  new_sector->ceiling = (model_t){0};

  // generate floor plane for sector
  generate_plane_with_norm(&new_sector->floor, (float2){w, d}, (float2){1.0f, 1.0f}, (float3){x + w * 0.5f, (float)floor_height, z + d * 0.5f}, (float3){0.0f, -1.0f, 0.0f});
  new_sector->floor.vertex_shader = &default_vertex_shader;
  new_sector->floor.frag_shader = &ground_frag;

  // generate ceiling plane for sector
  generate_plane_with_norm(&new_sector->ceiling, (float2){w, d}, (float2){1.0f, 1.0f}, (float3){x + w * 0.5f, (float)ceiling_height, z + d * 0.5f}, (float3){0.0f, 1.0f, 0.0f});
  new_sector->ceiling.vertex_shader = &default_vertex_shader;
  new_sector->ceiling.frag_shader = &ground_frag;

  // add one light in the center of the sector, only sometimes to avoid overkill
  if (world->num_lights < MAX_LIGHTS && rand() % 2 == 0) {
    world->lights[world->num_lights].position = (float3){ x + w * 0.5f, (float)floor_height + 1.0f, z + d * 0.5f };
    world->lights[world->num_lights].color = rgb_to_u32(255, 255, 255);
    world->lights[world->num_lights].is_directional = false;
    world->num_lights++;
  }

  if (world->tail) {
    world->tail->next = new_sector;
  } else {
    world->sectors = new_sector; // Set head if it's the first one
  }

  world->tail = new_sector;
  world->num_sectors++;
  return new_sector;
}

// Helper to find a sector sharing an edge with 'self'
sector_t* find_neighbor(world_t *world, sector_t *self, direction_t side) {
  sector_t *other = world->sectors;
  while (other) {
    if (other == self) {
      other = other->next;
      continue;
    }

    switch (side) {
      case DIR_NORTH:
        if (other->z == self->z + self->d &&
          other->x < self->x + self->w && other->x + other->w > self->x) return other;
        break;
      case DIR_SOUTH:
        if (other->z + other->d == self->z &&
          other->x < self->x + self->w && other->x + other->w > self->x) return other;
        break;
      case DIR_EAST:
          if (other->x == self->x + self->w &&
              other->z < self->z + self->d && other->z + other->d > self->z) return other;
          break;
      case DIR_WEST:
        if (other->x + other->w == self->x &&
          other->z < self->z + self->d && other->z + other->d > self->z) return other;
        break;
    }
    other = other->next;
  }
  return NULL;
}

// mallocs a new wall model and sets transforms
static void create_wall_segment(sector_t *s, float x1, float z1, float x2, float z2, float y_low, float y_high, float yaw) {
  s->num_walls++;
  s->walls = realloc(s->walls, s->num_walls * sizeof(model_t));
  model_t *m = &s->walls[s->num_walls - 1];
  *m = (model_t){0};

  float w = sqrtf(powf(x2 - x1, 2) + powf(z2 - z1, 2));
  float h = y_high - y_low;

  // generate a "neutral" plane facing up (0,1,0)
  generate_plane_with_norm(m, (float2){w, h}, (float2){1,1}, (float3){0,0,0}, (float3){0,1,0});

  m->transform.position = (float3){
    (x1 + x2) * 0.5f,
    (y_low + y_high) * 0.5f,
    (z1 + z2) * 0.5f
  };

  m->transform.pitch = PI / 2.0f; // Stand it up
  m->transform.yaw = yaw;         // Face it inward

  m->vertex_shader = &default_vertex_shader;
  m->frag_shader = &ground_frag;
}

// Determines if a wall is solid, split, or has a header
static void process_wall_side(world_t *world, sector_t *s, direction_t side, float wall_start, float wall_end, float fixed_pos, bool is_horizontal) {
  sector_t *n = find_neighbor(world, s, side);

  // static const float3 norms[] = { {0,0,-1}, {0,0,1}, {-1,0,0}, {1,0,0} };
  static const float yaws[]   = { 0, PI, PI/2.0f, -PI/2.0f };

  float yaw   = yaws[side];

  // Inner lambda-style macro to simplify coordinate mapping
  #define SPAWN_WALL(s1, s2, y_l, y_h) { \
      float _x1, _z1, _x2, _z2; \
      if (is_horizontal) { _x1 = s1; _x2 = s2; _z1 = fixed_pos; _z2 = fixed_pos; } \
      else { _z1 = s1; _z2 = s2; _x1 = fixed_pos; _x2 = fixed_pos; } \
      create_wall_segment(s, _x1, _z1, _x2, _z2, y_l, y_h, yaw); \
  }

  if (!n) {
      // No neighbor: generate full solid wall
      SPAWN_WALL(wall_start, wall_end, (float)s->floor_height, (float)s->ceiling_height);
  } else {
    float n_start = is_horizontal ? n->x : n->z;
    float n_end   = is_horizontal ? n->x + n->w : n->z + n->d;

    float overlap_1 = fmaxf(wall_start, n_start);
    float overlap_2 = fminf(wall_end, n_end);

    // left / bottom wing
    if (overlap_1 > wall_start) SPAWN_WALL(wall_start, overlap_1, (float)s->floor_height, (float)s->ceiling_height);

    // right / top wing
    if (overlap_2 < wall_end) SPAWN_WALL(overlap_2, wall_end, (float)s->floor_height, (float)s->ceiling_height);

    // header
    if (s->ceiling_height > n->ceiling_height) SPAWN_WALL(overlap_1, overlap_2, (float)n->ceiling_height, (float)s->ceiling_height);

    // footer
    if (n->floor_height > s->floor_height) SPAWN_WALL(overlap_1, overlap_2, (float)s->floor_height, (float)n->floor_height);
  }
  #undef SPAWN_WALL
}

static void add_entities(world_t *world) {
  sector_t *head = world->sectors;
  while (head) {
    if (world->num_entities < MAX_ENTITIES && rand() % 2 == 0) {
      entity_t *new_entity = &world->entities[world->num_entities];

      int x_off = (rand() % (head->w - 2)) + 1;
      int z_off = (rand() % (head->d - 2)) + 1;

      new_entity->current_sector = head;
      new_entity->transform.position = (float3){ head->x + x_off, head->floor_height + 1, head->z + z_off };
      generate_cube(&new_entity->mesh, new_entity->transform.position, (float3){1, 2, 1});

      new_entity->mesh.vertex_shader = &default_vertex_shader;
      new_entity->mesh.frag_shader = &default_lighting_frag_shader;
      new_entity->mesh.flat_color = rgb_to_u32(255, 0, 0);

      world->num_entities++;
    }

    head = head->next;
  }
}

void generate_random_map(world_t *world, int room_count) {
  // Start at origin
  int cur_x = 0;
  int cur_z = 0;

  // Default room size
  int r_w = 10;
  int r_d = 10;

  for (int i = 0; i < room_count; i++) {
    int f_h = (rand() % 3); // 0, 1, 2
    int c_h = f_h + 5 + (rand() % 3); // 4, 5, 6
    // 1. Try to add the sector.
    // add_sector should return NULL if a sector already exists at those coordinates
    // to prevent overlapping "glitch" rooms.
    sector_t *new_s = add_sector(world, cur_x - r_w/2, cur_z - r_d/2, r_w, r_d, c_h, f_h);

    // 2. Pick a random direction for the NEXT room
    direction_t move_dir = (direction_t)(rand() % 4);

    switch (move_dir) {
      case DIR_NORTH: cur_z += r_d; break;
      case DIR_SOUTH: cur_z -= r_d; break;
      case DIR_EAST:  cur_x += r_w; break;
      case DIR_WEST:  cur_x -= r_w; break;
    }

    // 3. Variety: Randomly change heights for "Steps" and "Headers"
    if (new_s) {
      new_s->floor_height = f_h;
      new_s->ceiling_height = c_h;
    }
  }

  finalize_world_geometry(world);
  add_entities(world);
}

void finalize_world_geometry(world_t *world) {
  for (sector_t *s = world->sectors; s != NULL; s = s->next) {
    // Clean up existing wall heap memory
    if (s->walls) {
      free(s->walls);
      s->walls = NULL;
      s->num_walls = 0;
    }

    // Process all 4 sides using the solver
    process_wall_side(world, s, DIR_NORTH, s->x, s->x + s->w, s->z + s->d, true);
    process_wall_side(world, s, DIR_SOUTH, s->x, s->x + s->w, s->z, true);
    process_wall_side(world, s, DIR_EAST,  s->z, s->z + s->d, s->x + s->w, false);
    process_wall_side(world, s, DIR_WEST,  s->z, s->z + s->d, s->x, false);
  }
}

void tick_entities(world_t *world, f32 delta_time) {
  for (usize i = 0; i < world->num_entities && world->num_entities > 0; i++) {
    entity_t *e = &world->entities[i];
  }
}

void render_world(renderer_t *renderer, world_t *world, transform_t *camera) {
  sector_t *sector = world->sectors;
  while (sector) {
    render_model(renderer, camera, &sector->floor, world->lights, world->num_lights);
    render_model(renderer, camera, &sector->ceiling, world->lights, world->num_lights);

    for (usize i = 0; i < sector->num_walls && sector->num_walls > 0; i++) {
      if (sector->walls[i].num_vertices > 0) {
        render_model(renderer, camera, &sector->walls[i], world->lights, world->num_lights);
      }
    }

    for (usize i = 0; i < world->num_entities && world->num_entities > 0; i++) {
      render_model(renderer, camera, &world->entities[i].mesh, world->lights, world->num_lights);
    }

    sector = sector->next;
  }
}
