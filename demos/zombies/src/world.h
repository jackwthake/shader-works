#ifndef __WORLD_H__
#define __WORLD_H__

#include <shader-works/renderer.h>

#define MAX_LIGHTS 5
#define MAX_ENTITIES 8

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
  float3 position;
  sector_t *current_sector;
  float radius;
  float floor_offset; // 2.0 for player eye-level, 1.0 for enemy center
  float y_velocity;
  bool active, is_grounded;
} physics_body_t;

typedef struct {
  physics_body_t body;
  model_t mesh;
  float3 target_pos;
} entity_t;

typedef struct {
  physics_body_t body;
  float move_speed;
  float mouse_sensitivity;
  float min_pitch, max_pitch;
  float ground_height;
  float delta_time;
  uint64_t last_frame_time;
  float bob_timer;
  bool is_moving;
  float current_fov_height;
  // Dash mechanics
  bool dash_available;
  float dash_timer;
  bool dash_active;
  float dash_duration;
  float3 dash_direction;
} fps_controller_t;

typedef struct {
  sector_t *sectors, *tail;
  light_t *lights;
  entity_t *entities;
  physics_body_t **entity_bodies;

  usize num_sectors, num_lights, num_entities;
} world_t;

void init_world(world_t *world);
void delete_world(world_t *world);

sector_t* find_neighbor(world_t *world, sector_t *self, direction_t side);
sector_t *point_in_sector(world_t *world, float3 position, float *floor_height);

void generate_random_map(world_t *world, int room_count);
void finalize_world_geometry(world_t *world);

void resolve_world_collision(world_t *world, physics_body_t *b, float3 move, float delta_time);
void tick_entities(world_t *world, fps_controller_t *controller, f32 delta_time);
void render_world(renderer_t *renderer, world_t *world, transform_t *camera);

// Generate UV coordinates for a model to use a specific region of the sprite atlas
// This function remaps all UV coordinates in a model to a specific sprite region.
// model: The model whose UVs will be remapped
// atlas_dim: Full dimensions of the sprite atlas in pixels (e.g., {256, 128})
// sprite_pos: Top-left corner of the sprite region in pixels (e.g., {0, 0})
// sprite_dim: Size of the sprite region in pixels (e.g., {32, 32})
void generate_model_uvs_from_atlas(model_t *model, float2 atlas_dim, float2 sprite_pos, float2 sprite_dim);

#endif // __WORLD_H__
