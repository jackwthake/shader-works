#include "scene.hpp"

#include <cstddef>
#include <cstdio>
#include <shader-works/maths.h>
#include <shader-works/renderer.h>
#include <shader-works/primitives.h>

#include "common/noise.h"

#define SEED 1

block_type_t Scene::map[Scene::MAP_WIDTH][Scene::MAP_DEPTH][Scene::MAP_HEIGHT];

// Destructor - clean up dynamically allocated resources
Scene::~Scene() {
  delete_model(&cube);
}

// Pre-computed UV coordinates for different block types
// Each cube has 6 faces * 2 triangles * 3 vertices = 36 UV coordinates
// Face order: bottom, top, front, back, right, left (matching cube_vertices order)

// Precomputed UV coordinates for each tile corner (static constant)
// Atlas is 80x24 pixels = 10 tiles x 3 rows, each tile is 8x8
// UV mapping: corner order is [bottom-left(0,0), bottom-right(1,0), top-right(1,1), top-left(0,1)]
static const float2 TILE_UVS[10][4] = {
  // Tile 0 (dirt) - row 0
  {{0.0f, 0.333333f}, {0.1f, 0.333333f}, {0.1f, 0.0f}, {0.0f, 0.0f}},
  // Tile 1 (grass side) - row 0
  {{0.1f, 0.333333f}, {0.2f, 0.333333f}, {0.2f, 0.0f}, {0.1f, 0.0f}},
  // Tile 2 (grass top) - row 0
  {{0.2f, 0.333333f}, {0.3f, 0.333333f}, {0.3f, 0.0f}, {0.2f, 0.0f}},
  // Tile 3 (stone) - row 0
  {{0.3f, 0.333333f}, {0.4f, 0.333333f}, {0.4f, 0.0f}, {0.3f, 0.0f}},
  // Tile 4 (sand) - row 0
  {{0.4f, 0.333333f}, {0.5f, 0.333333f}, {0.5f, 0.0f}, {0.4f, 0.0f}},
  // Tile 5 (wood) - row 0
  {{0.5f, 0.333333f}, {0.6f, 0.333333f}, {0.6f, 0.0f}, {0.5f, 0.0f}},
  // Tile 6 (leaves) - row 0
  {{0.6f, 0.333333f}, {0.7f, 0.333333f}, {0.7f, 0.0f}, {0.6f, 0.0f}},
  // Tile 7 (water) - row 0
  {{0.7f, 0.333333f}, {0.8f, 0.333333f}, {0.8f, 0.0f}, {0.7f, 0.0f}},
  // Tile 8 - row 1
  {{0.8f, 0.333333f}, {0.9f, 0.333333f}, {0.9f, 0.0f}, {0.8f, 0.0f}},
  // Tile 9 - row 1
  {{0.9f, 0.333333f}, {1.0f, 0.333333f}, {1.0f, 0.0f}, {0.9f, 0.0f}}
};

// Static UV coordinate arrays for different block types
float2 Scene::cube_uvs_grass[36];
float2 Scene::cube_uvs_stone[36];
float2 Scene::cube_uvs_dirt[36];
float2 Scene::cube_uvs_sand[36];
float2 Scene::cube_uvs_wood[36];
float2 Scene::cube_uvs_leaf[36];
float2 Scene::cube_uvs_water[36];

// Static helper functions

// Fast lookup function (no divisions, just array access)
static inline float2 get_tile_uv(int tile_id, int corner) {
  return TILE_UVS[tile_id][corner];
}

// Checks if block needs rendering (occlusion culling)
static bool is_block_visible(size_t x, size_t z, size_t y) {
  // If block is on the edge of the map, it's visible
  if (x == 0 || x == Scene::MAP_WIDTH-1 || z == 0 || z == Scene::MAP_DEPTH-1 || y == 0 || y == Scene::MAP_HEIGHT-1) {
    return true;
  }

  // Check if all 6 adjacent blocks are solid (occluding this block)
  return (Scene::map[x-1][z][y] == block_type_t::AIR ||
          Scene::map[x+1][z][y] == block_type_t::AIR ||
          Scene::map[x][z-1][y] == block_type_t::AIR ||
          Scene::map[x][z+1][y] == block_type_t::AIR ||
          Scene::map[x][z][y-1] == block_type_t::AIR ||
          Scene::map[x][z][y+1] == block_type_t::AIR);
}

// Generate UV coordinates for a full cube (6 faces, each with 2 triangles)
// Actual face order from primitives.c: Front(-Z), Back(+Z), Left(+X), Right(-X), Top(+Y), Bottom(-Y)
static void generate_cube_uvs(float2* uvs, int top_tile, int bottom_tile, int side_tile) {
  int uv_idx = 0;

  // Face 0: Front -Z (vertices 0-5)
  uvs[uv_idx++] = get_tile_uv(side_tile, 0);
  uvs[uv_idx++] = get_tile_uv(side_tile, 1);
  uvs[uv_idx++] = get_tile_uv(side_tile, 2);
  uvs[uv_idx++] = get_tile_uv(side_tile, 0);
  uvs[uv_idx++] = get_tile_uv(side_tile, 2);
  uvs[uv_idx++] = get_tile_uv(side_tile, 3);

  // Face 1: Back +Z (vertices 6-11)
  uvs[uv_idx++] = get_tile_uv(side_tile, 0);
  uvs[uv_idx++] = get_tile_uv(side_tile, 1);
  uvs[uv_idx++] = get_tile_uv(side_tile, 2);
  uvs[uv_idx++] = get_tile_uv(side_tile, 0);
  uvs[uv_idx++] = get_tile_uv(side_tile, 2);
  uvs[uv_idx++] = get_tile_uv(side_tile, 3);

  // Face 2: Left +X (vertices 12-17)
  uvs[uv_idx++] = get_tile_uv(side_tile, 0);
  uvs[uv_idx++] = get_tile_uv(side_tile, 1);
  uvs[uv_idx++] = get_tile_uv(side_tile, 2);
  uvs[uv_idx++] = get_tile_uv(side_tile, 0);
  uvs[uv_idx++] = get_tile_uv(side_tile, 2);
  uvs[uv_idx++] = get_tile_uv(side_tile, 3);

  // Face 3: Right -X (vertices 18-23)
  uvs[uv_idx++] = get_tile_uv(side_tile, 0);
  uvs[uv_idx++] = get_tile_uv(side_tile, 1);
  uvs[uv_idx++] = get_tile_uv(side_tile, 2);
  uvs[uv_idx++] = get_tile_uv(side_tile, 0);
  uvs[uv_idx++] = get_tile_uv(side_tile, 2);
  uvs[uv_idx++] = get_tile_uv(side_tile, 3);

  // Face 4: Top +Y (vertices 24-29)
  uvs[uv_idx++] = get_tile_uv(top_tile, 0);
  uvs[uv_idx++] = get_tile_uv(top_tile, 1);
  uvs[uv_idx++] = get_tile_uv(top_tile, 2);
  uvs[uv_idx++] = get_tile_uv(top_tile, 0);
  uvs[uv_idx++] = get_tile_uv(top_tile, 2);
  uvs[uv_idx++] = get_tile_uv(top_tile, 3);

  // Face 5: Bottom -Y (vertices 30-35)
  uvs[uv_idx++] = get_tile_uv(bottom_tile, 0);
  uvs[uv_idx++] = get_tile_uv(bottom_tile, 1);
  uvs[uv_idx++] = get_tile_uv(bottom_tile, 2);
  uvs[uv_idx++] = get_tile_uv(bottom_tile, 0);
  uvs[uv_idx++] = get_tile_uv(bottom_tile, 2);
  uvs[uv_idx++] = get_tile_uv(bottom_tile, 3);
}

static size_t terrain_height(size_t x, size_t z, size_t max_height) {
  // fbm returns [-1, 1], shift to [0, 1], then scale to terrain height range
  float noise = fbm((float)x * 0.1f, (float)z * 0.1f, 5, SEED);
  float height = (noise * 0.5f + 0.5f) * (max_height - 3);  // Map [-1,1] to [0, max_height-3]

  if (height < 0) return 0;
  if (height > max_height - 1) return max_height - 1;

  return (size_t)height;
}


// Initialize the scene, load resources, etc.
void Scene::init() {
  // Initialize UV coordinates for different block types
  generate_cube_uvs(cube_uvs_grass, 2, 0, 1);  // grass: top=grass(2), bottom=dirt(0), sides=dirt(1)
  generate_cube_uvs(cube_uvs_stone, 3, 3, 3);  // stone: all faces stone(3)
  generate_cube_uvs(cube_uvs_dirt, 0, 0, 0);   // dirt: all faces dirt(0)
  generate_cube_uvs(cube_uvs_sand, 4, 4, 4);   // sand: all faces sand(4)
  generate_cube_uvs(cube_uvs_wood, 5, 5, 5);   // Wood: all faces bark(5) TODO: add bottom and top texture
  generate_cube_uvs(cube_uvs_leaf, 6, 6, 6);   // Leaf: all faces Leaves(6)
  generate_cube_uvs(cube_uvs_water, 7, 7, 7);  // Water: all faces Water(6) NOTE: Maybe just render the top face?

  for (size_t x = 0; x < MAP_WIDTH; ++x) {
    for (size_t z = 0; z < MAP_DEPTH; ++z) {
      float height = terrain_height(x, z, MAP_HEIGHT);
      for (size_t y = 0; y < MAP_HEIGHT; ++y) {
        if (y < height && y > height - 3) { // Dirt
          map[x][z][y] = block_type_t::DIRT;
        } else if (y < height) { // Stone
          map[x][z][y] = block_type_t::STONE;
        } else if (y == height) {
          map[x][z][y] = block_type_t::GRASS;
        } else {
          map[x][z][y] = block_type_t::AIR;
        }
      }
    }
  }

  player_cam = {0};
  player_cam.position.y = (float)terrain_height(0, 0, MAP_HEIGHT) + 2.0f;  // Spawn 2 blocks above terrain
  fps_controller.ground_height = player_cam.position.y;

  // Initialize the cube mesh
  generate_cube(&cube, {0.f, 0.f, 0.f}, {1.f, 1.f, 1.f});
  cube.vertex_shader = nullptr;
  cube.frag_shader = nullptr;
  cube.use_textures = true;

  fprintf(stderr, "Cube initialized: vertices=%zu, vertex_data=%p\n", cube.num_vertices, cube.vertex_data);
  fflush(stderr);
}

// Update the scene (e.g., animations, physics)
void Scene::update(float delta_time) {
  update_timing(fps_controller);
  handle_input(fps_controller, player_cam);

  // Clamp player position to map bounds
  int px = (int)player_cam.position.x;
  int pz = (int)player_cam.position.z;
  if (px < 0) px = 0;
  if (px >= MAP_WIDTH) px = MAP_WIDTH - 1;
  if (pz < 0) pz = 0;
  if (pz >= MAP_DEPTH) pz = MAP_DEPTH - 1;

  // Find the highest non-air block at this position
  int ground_level = 0;
  for (int y = MAP_HEIGHT - 1; y >= 0; --y) {
    if (map[px][pz][y] != block_type_t::AIR) {
      ground_level = y + 1; // Stand on top of the block
      break;
    }
  }

  fps_controller.ground_height = (float)ground_level;
  player_cam.position.y = fps_controller.ground_height + 2.0f;
}

// Render the scene to the display buffer
void Scene::render(renderer_t &state, uint32_t *buffer, float *depth_buffer) {
  // Debug check
  static int frame = 0;
  if (frame == 0) {
    fprintf(stderr, "First render: this=%p, cube.num_vertices=%zu, cube.vertex_data=%p, &(this->cube)=%p\n",
            this, cube.num_vertices, cube.vertex_data, &(this->cube));
    fflush(stderr);
  }
  frame++;

  // Simple frustum culling: only render blocks near the player
  const float render_distance = 16.0f;
  int min_x = (int)(player_cam.position.x - render_distance);
  int max_x = (int)(player_cam.position.x + render_distance);
  int min_z = (int)(player_cam.position.z - render_distance);
  int max_z = (int)(player_cam.position.z + render_distance);

  if (min_x < 0) min_x = 0;
  if (max_x >= MAP_WIDTH) max_x = MAP_WIDTH - 1;
  if (min_z < 0) min_z = 0;
  if (max_z >= MAP_DEPTH) max_z = MAP_DEPTH - 1;

  // Copy UVs into the vertex data (for now just use grass)
  for (size_t i = 0; i < cube.num_vertices; ++i) {
    cube.vertex_data[i].uv = cube_uvs_grass[i];
  }

  int blocks_rendered = 0;
  for (int x = min_x; x <= max_x; ++x) {
    for (int z = min_z; z <= max_z; ++z) {
      float height = terrain_height(x, z, MAP_HEIGHT);

      for (size_t y = 0; y <= height; ++y) {
        if (is_block_visible(x, z, y)) {
          block_type_t block = Scene::map[x][z][y];

          cube.transform.position = { (float)x, (float)y, (float)z };

          // Select the appropriate UV coordinates based on block type
          float2 *uvs = cube_uvs_stone; // Default
          switch (block) {
          case block_type_t::GRASS:
            uvs = cube_uvs_grass;
            break;
          case block_type_t::STONE:
            uvs = cube_uvs_stone;
            break;
          case block_type_t::DIRT:
            uvs = cube_uvs_dirt;
            break;
          case block_type_t::SAND:
            uvs = cube_uvs_sand;
            break;
          case block_type_t::WOOD:
            uvs = cube_uvs_wood;
            break;
          case block_type_t::LEAVES:
            uvs = cube_uvs_leaf;
            break;
          case block_type_t::WATER:
            uvs = cube_uvs_water;
            break;
          default:
            uvs = cube_uvs_stone;
            break;
          }

          // Copy UVs to all vertices
          for (size_t i = 0; i < cube.num_vertices; ++i) {
            cube.vertex_data[i].uv = uvs[i];
          }

          render_model(&state, &player_cam, &cube, &sun, 1);
        }
      }
    }
  }

  apply_fog_to_screen(&state, 5, 15, 50, 50, 175);
}
