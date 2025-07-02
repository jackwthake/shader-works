#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

#include "util/maths.h"
#include "display.h"

enum class block_type_t : uint8_t {
  AIR = 0,
  STONE = 1,
  DIRT = 2,
  GRASS = 3,
  SAND = 4,
  WATER = 5,
  WOOD = 6,
  LEAVES = 7,
  STONE_BRICKS = 8,
  COBBLESTONE = 9,
};


struct Model {
  float3 *vertices; // List of vertices
  std::vector<float2> uvs; // texture coordinates
  
  float3 scale;
  Transform transform;

  Model() : vertices(nullptr), scale(1, 1, 1) {}
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
  private:
    static constexpr size_t MAP_WIDTH = 8;
    static constexpr size_t MAP_DEPTH = 8;
    static constexpr size_t MAP_HEIGHT = 8;

    void generate_terrain();

    Transform camera; // Camera transform for the scene
    block_type_t map[MAP_WIDTH][MAP_DEPTH][MAP_HEIGHT];

    // Definition of cube vertices array
    static float3 cube_vertices[36];
};