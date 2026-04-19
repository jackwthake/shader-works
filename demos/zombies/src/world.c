#include "world.h"

#include <stdlib.h>
#include <math.h>

#include <SDL3/SDL.h>
#include <shader-works/maths.h>

#include "world.h"

extern fragment_shader_t light_and_dither;
extern fragment_shader_t ground_shader;
extern renderer_t renderer_state;

void get_uv_from_atlas(float2 *uv, float2 atlas_dim, float2 sprite_pos, float2 sprite_dim) {
  uv->x = sprite_pos.x / atlas_dim.x;
  uv->y = sprite_pos.y / atlas_dim.y;
  uv->x += 0.5f / atlas_dim.x; // Add half-pixel offset to sample center of texel
  uv->y += 0.5f / atlas_dim.y;
}

// Generate UV coordinates for a model to use a specific region of the sprite atlas
// This function remaps all UV coordinates in a model to a specific sprite region.
//
// model: The model whose UVs will be remapped
// atlas_dim: Full dimensions of the sprite atlas in pixels (e.g., {256, 128})
// sprite_pos: Top-left corner of the sprite region in pixels (e.g., {0, 0})
// sprite_dim: Size of the sprite region in pixels (e.g., {32, 32})
void generate_model_uvs_from_atlas(model_t *model, float2 atlas_dim, float2 sprite_pos, float2 sprite_dim) {
  if (!model || model->num_vertices == 0) return;

  // Calculate normalized sprite bounds (0 to 1 in atlas space)
  float sprite_uv_min_x = sprite_pos.x / atlas_dim.x;
  float sprite_uv_min_y = sprite_pos.y / atlas_dim.y;
  float sprite_uv_max_x = (sprite_pos.x + sprite_dim.x) / atlas_dim.x;
  float sprite_uv_max_y = (sprite_pos.y + sprite_dim.y) / atlas_dim.y;

  // Remap all model UVs from [0,1] to the sprite region
  for (usize i = 0; i < model->num_vertices; i++) {
    float u = model->vertex_data[i].uv.x;
    float v = model->vertex_data[i].uv.y;

    // Map from [0,1] to sprite region bounds
    model->vertex_data[i].uv.x = sprite_uv_min_x + u * (sprite_uv_max_x - sprite_uv_min_x);
    model->vertex_data[i].uv.y = sprite_uv_min_y + v * (sprite_uv_max_y - sprite_uv_min_y);
  }
}

void init_world(world_t *world) {
  world->sectors = NULL;
  world->tail = NULL;
  world->num_sectors = 0;
  world->lights = calloc(MAX_LIGHTS, sizeof(light_t));
  world->num_lights = 0;
  world->entities = calloc(MAX_ENTITIES, sizeof(entity_t));
  world->num_entities = 0;
  world->entity_bodies = calloc(MAX_ENTITIES + 1, sizeof(physics_body_t *));
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

  if (world->entity_bodies) {
    free(world->entity_bodies);
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
  new_sector->floor.frag_shader = &ground_shader;
  new_sector->floor.use_textures = false;

  // Set UVs for floor to grab first 32x32 pixels of atlas (the "base" texture)
  generate_model_uvs_from_atlas(&new_sector->floor, renderer_state.atlas_dim, make_float2(32.0f, 32.0f), make_float2(16.0f, 16.0f));

  // generate ceiling plane for sector
  generate_plane_with_norm(&new_sector->ceiling, (float2){w, d}, (float2){1.0f, 1.0f}, (float3){x + w * 0.5f, (float)ceiling_height, z + d * 0.5f}, (float3){0.0f, 1.0f, 0.0f});
  new_sector->ceiling.vertex_shader = &default_vertex_shader;
  new_sector->ceiling.frag_shader = &ground_shader;
  new_sector->ceiling.use_textures = false;

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

sector_t *point_in_sector(world_t *world, float3 position, float *floor_height) {
  // Brute-force search all sectors for one that contains the point
  sector_t *s = world->sectors;
  while (s) {
    if (position.x >= s->x && position.x <= s->x + s->w) {
      if (position.z >= s->z && position.z <= s->z + s->d) {
        if (floor_height) {
          *floor_height = (float)s->floor_height;
        }
        return s;
      }
    }
    s = s->next;
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
  m->frag_shader = &ground_shader;
  m->use_textures = false;
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

      new_entity->body.current_sector = head;
      new_entity->body.radius = 0.5f;
      new_entity->body.floor_offset = 1.0f; // Enemy center, not
      new_entity->body.position = (float3){ head->x + x_off, head->floor_height + new_entity->body.floor_offset, head->z + z_off };
      load_obj_model(&new_entity->mesh, "res/skeleton.obj", new_entity->body.position, 0.5f, true);

      generate_model_uvs_from_atlas(&new_entity->mesh, renderer_state.atlas_dim, make_float2(32.0f, 0.0f), make_float2(32.0f, 32.0f));

      new_entity->mesh.vertex_shader = &default_vertex_shader;
      new_entity->mesh.frag_shader = &light_and_dither;
      new_entity->target_pos = new_entity->body.position;
      new_entity->body.active = true;

      world->entity_bodies[world->num_entities] = &new_entity->body;
      world->num_entities++;
    }

    head = head->next;
  }

  if (world->num_entities != MAX_ENTITIES) {
    add_entities(world); // Keep trying to add entities until we hit the max, since it's random
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

void resolve_world_collision(world_t *world, physics_body_t *b, float3 move, float delta_time) {
  float gravity = -15.5f;
  sector_t *s = b->current_sector;
  float padding = b->radius;

  // 1. Vertical Physics
  b->y_velocity += gravity * delta_time;
  b->position.y += fmaxf(b->y_velocity * delta_time, b->current_sector->floor_height + b->floor_offset - b->position.y);

  // 2. Initial Horizontal Movement
  float next_x = b->position.x + move.x;
  float next_z = b->position.z + move.z;

  // 3. Entity-to-Entity Collision (XZ Plane)
  // We check MAX_ENTITIES + 1 because the player is at the last index.
  for (usize i = 0; i < MAX_ENTITIES + 1; i++) {
    physics_body_t *other = world->entity_bodies[i];

    // Skip if the body is null, inactive, or is the current body we are processing
    if (!other || !other->active || other == b) continue;

    // Calculate distance between centers on the XZ plane
    float dx = next_x - other->position.x;
    float dz = next_z - other->position.z;
    float distance_sq = (dx * dx) + (dz * dz);
    float min_dist = b->radius + other->radius;

    if (distance_sq < (min_dist * min_dist)) {
      // Collision detected! Resolve by pushing 'b' away from 'other'
      float distance = sqrtf(distance_sq);

      // Prevent division by zero if they are exactly on top of each other
      if (distance == 0.0f) {
        next_x += 0.01f; // Slight nudge
      } else {
        float overlap = min_dist - distance;
        // Push the moving body out of the other body
        next_x += (dx / distance) * overlap;
        next_z += (dz / distance) * overlap;
      }
    }
  }

  // 4. Sector/Wall Collision (Using the updated next_x/next_z)
  // X-Axis Wall/Sector Check
  if (next_x < s->x + padding || next_x > s->x + s->w - padding) {
    direction_t dir = (next_x < s->x + padding) ? DIR_WEST : DIR_EAST;
    sector_t *n = find_neighbor(world, s, dir);

    if (n && n->floor_height <= b->position.y + 0.5f) {
      if (next_x < n->x + n->w && next_x > n->x) s = n;
    } else {
      next_x = b->position.x; // Block movement
    }
  }
  b->position.x = next_x;

  // Z-Axis Wall/Sector Check
  if (next_z < s->z + padding || next_z > s->z + s->d - padding) {
    direction_t dir = (next_z < s->z + padding) ? DIR_SOUTH : DIR_NORTH;
    sector_t *n = find_neighbor(world, s, dir);

    if (n && n->floor_height <= b->position.y + 0.5f) {
      if (next_z < n->z + n->d && next_z > n->z) s = n;
    } else {
      next_z = b->position.z; // Block movement
    }
  }
  b->position.z = next_z;

  b->current_sector = s;
}

#define MAX_PATH_DEPTH 4
int ai_path_find_sector(world_t *world, sector_t *s, physics_body_t *body, sector_t **built_path, int depth) {
  if (!s || !body || !built_path || depth >= MAX_PATH_DEPTH || s->visited) return -1;

  float3 pos = body->position;

  // Mark this sector as visited in this search
  s->visited = true;

  // add ourselves to our prospective path
  built_path[depth] = s;

  // check if the player is in this sector (match controller.c sector detection bounds)
  if (pos.x >= s->x && pos.x <= s->x + s->w) {
    if (pos.z >= s->z && pos.z <= s->z + s->d) {
      return depth;
    }
  }

  // not a hit, check neighbors at next depth level
  int ret;
  int next_depth = depth + 1;

  if ((ret = ai_path_find_sector(world, find_neighbor(world, s, DIR_NORTH), body, built_path, next_depth)) != -1)
    return ret;

  if ((ret = ai_path_find_sector(world, find_neighbor(world, s, DIR_EAST), body, built_path, next_depth)) != -1)
    return ret;

  if ((ret = ai_path_find_sector(world, find_neighbor(world, s, DIR_SOUTH), body, built_path, next_depth)) != -1)
    return ret;

  if ((ret = ai_path_find_sector(world, find_neighbor(world, s, DIR_WEST), body, built_path, next_depth)) != -1)
    return ret;

  return -1;
}

void tick_entities(world_t *world, fps_controller_t *controller, float delta_time) {
  static sector_t *paths[MAX_ENTITIES][MAX_PATH_DEPTH] = { 0 };

  for (usize i = 0; i < world->num_entities; i++) {
    entity_t *ent = &world->entities[i];
    // if (!ent->active) continue;

    int depth = ai_path_find_sector(world, ent->body.current_sector, (physics_body_t *)controller, paths[i], 0);

    // Clear visited flags for all sectors
    sector_t *s = world->sectors;
    while (s) {
      s->visited = false;
      s = s->next;
    }

    // Update target only when we find a path
    if (depth != -1) {
      if (depth == 0) {
        // Player is in the same sector as the entity, so target the player position
        ent->target_pos = ((physics_body_t *)controller)->position;
      } else {
        sector_t *target = paths[i][1];
        ent->target_pos = (float3){
          target->x + (target->w / 2),
          ent->body.current_sector->floor_height + 1,
          target->z + (target->d / 2)
        };
      }
    }

    // Always move toward the target
    float3 dir_vec = float3_sub(ent->target_pos, ent->body.position);
    dir_vec.y = 0;

    if (float3_magnitude(dir_vec) > 0.1f) {
      dir_vec = float3_normalize(dir_vec);
      float3 move_vec = float3_scale(dir_vec, 3.5f * delta_time);
      resolve_world_collision(world, (physics_body_t*)ent, move_vec, delta_time);
      ent->mesh.transform.position = ent->body.position;
    }
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
