#pragma once

#include <cstdint>
#include <cstddef>

#include "util/maths.h"
#include "display.h"

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


struct Model {
  float3 *vertices; // List of vertices
  float2 *uvs; // pointer to texture coordinates (static data)
  
  float3 scale;
  Transform transform;

  Model() : vertices(nullptr), uvs(nullptr), scale(1, 1, 1) {}
};


class Scene {
  public:
    Scene() = default;
    ~Scene() = default;

    // Initialize the scene, load resources, etc.
    void init();

    // Update the scene (e.g., animations, physics)
    void update(float delta_time);

    // Render the scene to the display buffer
    void render(uint16_t *buffer, float *depth_buffer);

    static constexpr size_t MAP_WIDTH = 32;
    static constexpr size_t MAP_DEPTH = 32;
    static constexpr size_t MAP_HEIGHT = 16;

		//										 [    x    ][    z    ][    y     ]
		static block_type_t map[MAP_WIDTH][MAP_DEPTH][MAP_HEIGHT];
  private:
    void generate_terrain();

    Transform camera; // Camera transform for the scene

    // Definition of cube vertices array
    static float3 cube_vertices[36];
    
    // Pre-computed UV coordinates for different tile types
    static float2 cube_uvs_grass[36];   // tile_id = 3 (grass)
    static float2 cube_uvs_stone[36];   // tile_id = 1 (stone)
    static float2 cube_uvs_dirt[36];    // tile_id = 2 (dirt)
    static float2 cube_uvs_sand[36];    // tile_id = 4 (sand)
    static float2 cube_uvs_wood[36];    // tile_id = 5 (wood)
    static float2 cube_uvs_leaf[36];    // tile_id = 6 (leaf)
    static float2 cube_uvs_water[36];    // tile_id = 7 (water)
};
