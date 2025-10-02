#ifndef SCENE_HPP
#define SCENE_HPP

#include <stdint.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <shader-works/maths.h>
#include <shader-works/renderer.h>

#ifdef __cplusplus
}
#endif

enum class block_type_t : uint8_t {
  AIR = 0,
  STONE = 1,
  DIRT = 2,
  GRASS = 3,
  SAND = 4,
  WOOD = 5,
  LEAVES = 6,
  WATER = 7,
  STONE_BRICKS = 8,
  COBBLESTONE = 9,
};

struct fps_controller_t {
  float move_speed;
  float mouse_sensitivity;
  float min_pitch, max_pitch;
  float ground_height;
  float delta_time;
  uint64_t last_frame_time;
};

class Scene {
public:
  Scene() = default;
  ~Scene();  // Need to clean up cube model

  // Initialize the scene, load resources, etc.
  void init();

  // Update the scene (e.g., animations, physics)
  void update(float delta_time);

  // Render the scene to the display buffer
  void render(renderer_t &state, uint32_t *buffer, float *depth_buffer);

  static constexpr size_t MAP_WIDTH = 32;
  static constexpr size_t MAP_DEPTH = 32;
  static constexpr size_t MAP_HEIGHT = 16;

  //										 [    x    ][    z    ][    y     ]
  static block_type_t map[MAP_WIDTH][MAP_DEPTH][MAP_HEIGHT];
private:
  fps_controller_t fps_controller = {
    .move_speed = 8.0f,
    .mouse_sensitivity = 0.0015f,
    .min_pitch = -M_PI/2 + 0.1f,
    .max_pitch = M_PI/2 - 0.1f,
    .ground_height = 2.0f,
  };

  transform_t player_cam;
  model_t cube = {};  // Zero-initialize

  light_t sun = {
    .direction = {1, -1, 1},
    .color = 0xFFFFFFFF,  // White color (RGBA)
    .is_directional = true
  };

  // Definition of cube vertices array
  static float3 cube_vertices[36];

  // Pre-computed UV coordinates for different tile types
  static float2 cube_uvs_grass[36];   // tile_id = 3 (grass)
  static float2 cube_uvs_stone[36];   // tile_id = 1 (stone)
  static float2 cube_uvs_dirt[36];    // tile_id = 2 (dirt)
  static float2 cube_uvs_sand[36];    // tile_id = 4 (sand)
  static float2 cube_uvs_wood[36];    // tile_id = 5 (wood)
  static float2 cube_uvs_leaf[36];    // tile_id = 6 (leaf)
  static float2 cube_uvs_water[36];   // tile_id = 7 (water)
};

// Forward declarations for platform-specific functions
void update_timing(fps_controller_t &controller);
void handle_input(fps_controller_t &controller, transform_t &camera);

#endif
