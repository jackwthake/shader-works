#include "scene.h"

#include "device.h"
#include "renderer.h"

static block_type_t _map[Scene::MAP_WIDTH][Scene::MAP_DEPTH][Scene::MAP_HEIGHT];

// Helper function to check if block needs rendering (occlusion culling)
bool is_block_visible(size_t x, size_t z, size_t y) {
  // If block is on the edge of the map, it's visible
  if (x == 0 || x == Scene::MAP_WIDTH-1 || z == 0 || z == Scene::MAP_DEPTH-1 || y == 0 || y == Scene::MAP_HEIGHT-1) {
    return true;
  }
  
  // Check if all 6 adjacent blocks are solid (occluding this block)
  return (_map[x-1][z][y] == block_type_t::AIR ||
          _map[x+1][z][y] == block_type_t::AIR ||
          _map[x][z-1][y] == block_type_t::AIR ||
          _map[x][z+1][y] == block_type_t::AIR ||
          _map[x][z][y-1] == block_type_t::AIR ||
          _map[x][z][y+1] == block_type_t::AIR);
}

float3 Scene::cube_vertices[36] = {
  {-1.000000, 1.000000, -1.000000}, {-1.000000, 1.000000, 1.000000}, {1.000000, 1.000000, 1.000000}, 
  {-1.000000, 1.000000, -1.000000}, {1.000000, 1.000000, 1.000000}, {1.000000, 1.000000, -1.000000}, 
  {1.000000, -1.000000, -1.000000}, {1.000000, -1.000000, 1.000000}, {-1.000000, -1.000000, 1.000000}, 
  {1.000000, -1.000000, -1.000000}, {-1.000000, -1.000000, 1.000000}, {-1.000000, -1.000000, -1.000000}, 
  {1.000000, 1.000000, 1.000000}, {1.000000, -1.000000, 1.000000}, {-1.000000, -1.000000, 1.000000}, 
  {1.000000, 1.000000, 1.000000}, {-1.000000, -1.000000, 1.000000}, {-1.000000, 1.000000, 1.000000}, 
  {-1.000000, 1.000000, -1.000000}, {-1.000000, -1.000000, -1.000000}, {1.000000, -1.000000, -1.000000}, 
  {-1.000000, 1.000000, -1.000000}, {1.000000, -1.000000, -1.000000}, {1.000000, 1.000000, -1.000000}, 
  {1.000000, 1.000000, -1.000000}, {1.000000, -1.000000, -1.000000}, {1.000000, -1.000000, 1.000000}, 
  {1.000000, 1.000000, -1.000000}, {1.000000, -1.000000, 1.000000}, {1.000000, 1.000000, 1.000000}, 
  {-1.000000, 1.000000, 1.000000}, {-1.000000, -1.000000, 1.000000}, {-1.000000, -1.000000, -1.000000}, 
  {-1.000000, 1.000000, 1.000000}, {-1.000000, -1.000000, -1.000000}, {-1.000000, 1.000000, -1.000000}
};


// Pre-computed UV coordinates for different block types
// Each cube has 6 faces * 2 triangles * 3 vertices = 36 UV coordinates
// Face order: bottom, top, front, back, right, left (matching cube_vertices order)

// Precomputed UV coordinates for each tile corner (static constant)
static const float2 TILE_UVS[10][4] = {
  // Tile 0 (dirt)
  {{0.0f, 0.333333f}, {0.0f, 0.0f}, {0.1f, 0.0f}, {0.1f, 0.333333f}},
  // Tile 1 (grass side) 
  {{0.1f, 0.333333f}, {0.1f, 0.0f}, {0.2f, 0.0f}, {0.2f, 0.333333f}},
  // Tile 2 (grass top)
  {{0.2f, 0.333333f}, {0.2f, 0.0f}, {0.3f, 0.0f}, {0.3f, 0.333333f}},
  // Tile 3 (stone)
  {{0.3f, 0.333333f}, {0.3f, 0.0f}, {0.4f, 0.0f}, {0.4f, 0.333333f}},
  // Tile 4
  {{0.4f, 0.333333f}, {0.4f, 0.0f}, {0.5f, 0.0f}, {0.5f, 0.333333f}},
  // Tile 5
  {{0.5f, 0.333333f}, {0.5f, 0.0f}, {0.6f, 0.0f}, {0.6f, 0.333333f}},
  // Tile 6
  {{0.6f, 0.333333f}, {0.6f, 0.0f}, {0.7f, 0.0f}, {0.7f, 0.333333f}},
  // Tile 7
  {{0.7f, 0.333333f}, {0.7f, 0.0f}, {0.8f, 0.0f}, {0.8f, 0.333333f}},
  // Tile 8
  {{0.8f, 0.333333f}, {0.8f, 0.0f}, {0.9f, 0.0f}, {0.9f, 0.333333f}},
  // Tile 9
  {{0.9f, 0.333333f}, {0.9f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.333333f}}
};

// Fast lookup function (no divisions, just array access)
inline float2 get_tile_uv(int tile_id, int corner) {
  return TILE_UVS[tile_id][corner];
}

// Generate UV coordinates for a full cube (6 faces, each with 2 triangles)
void generate_cube_uvs(float2* uvs, int top_tile, int bottom_tile, int side_tile) {
  int uv_idx = 0;
  
  // Face 1: Bottom (triangles 0-1)
  uvs[uv_idx++] = get_tile_uv(bottom_tile, 0); // top-left
  uvs[uv_idx++] = get_tile_uv(bottom_tile, 1); // bottom-left  
  uvs[uv_idx++] = get_tile_uv(bottom_tile, 2); // bottom-right
  uvs[uv_idx++] = get_tile_uv(bottom_tile, 0); // top-left
  uvs[uv_idx++] = get_tile_uv(bottom_tile, 2); // bottom-right
  uvs[uv_idx++] = get_tile_uv(bottom_tile, 3); // top-right
  
  // Face 2: Top (triangles 2-3)
  uvs[uv_idx++] = get_tile_uv(top_tile, 0);
  uvs[uv_idx++] = get_tile_uv(top_tile, 1);
  uvs[uv_idx++] = get_tile_uv(top_tile, 2);
  uvs[uv_idx++] = get_tile_uv(top_tile, 0);
  uvs[uv_idx++] = get_tile_uv(top_tile, 2);
  uvs[uv_idx++] = get_tile_uv(top_tile, 3);
  
  // Face 3: Front (triangles 4-5)
  uvs[uv_idx++] = get_tile_uv(side_tile, 0);
  uvs[uv_idx++] = get_tile_uv(side_tile, 1);
  uvs[uv_idx++] = get_tile_uv(side_tile, 2);
  uvs[uv_idx++] = get_tile_uv(side_tile, 0);
  uvs[uv_idx++] = get_tile_uv(side_tile, 2);
  uvs[uv_idx++] = get_tile_uv(side_tile, 3);
  
  // Face 4: Back (triangles 6-7)
  uvs[uv_idx++] = get_tile_uv(side_tile, 0);
  uvs[uv_idx++] = get_tile_uv(side_tile, 1);
  uvs[uv_idx++] = get_tile_uv(side_tile, 2);
  uvs[uv_idx++] = get_tile_uv(side_tile, 0);
  uvs[uv_idx++] = get_tile_uv(side_tile, 2);
  uvs[uv_idx++] = get_tile_uv(side_tile, 3);
  
  // Face 5: Right (triangles 8-9)
  uvs[uv_idx++] = get_tile_uv(side_tile, 0);
  uvs[uv_idx++] = get_tile_uv(side_tile, 1);
  uvs[uv_idx++] = get_tile_uv(side_tile, 2);
  uvs[uv_idx++] = get_tile_uv(side_tile, 0);
  uvs[uv_idx++] = get_tile_uv(side_tile, 2);
  uvs[uv_idx++] = get_tile_uv(side_tile, 3);
  
  // Face 6: Left (triangles 10-11)
  uvs[uv_idx++] = get_tile_uv(side_tile, 0);
  uvs[uv_idx++] = get_tile_uv(side_tile, 1);
  uvs[uv_idx++] = get_tile_uv(side_tile, 2);
  uvs[uv_idx++] = get_tile_uv(side_tile, 0);
  uvs[uv_idx++] = get_tile_uv(side_tile, 2);
  uvs[uv_idx++] = get_tile_uv(side_tile, 3);
}

// Static UV coordinate arrays for different block types
float2 Scene::cube_uvs_grass[36];
float2 Scene::cube_uvs_stone[36];  
float2 Scene::cube_uvs_dirt[36];


// Initialize the scene, load resources, etc.
void Scene::init() {
  // Initialize UV coordinates for different block types
  generate_cube_uvs(cube_uvs_grass, 2, 0, 1);  // grass: top=grass(2), bottom=dirt(0), sides=dirt(1)
  generate_cube_uvs(cube_uvs_stone, 3, 3, 3);  // stone: all faces stone(3)
  generate_cube_uvs(cube_uvs_dirt, 0, 0, 0);   // dirt: all faces dirt(0)

  for (size_t x = 0; x < MAP_WIDTH; ++x) {
    for (size_t z = 0; z < MAP_DEPTH; ++z) {
      for (size_t y = 0; y < MAP_HEIGHT; ++y) {
        _map[x][z][y] = block_type_t::AIR; // Initialize all blocks to AIR
      }
    }
  }

  generate_terrain();

  camera.position = { 5.0f, 5.0f, 10.0f }; // Set initial camera position
  camera.yaw = -45 * DEG_TO_RAD; // Set initial yaw to 45 degrees
  camera.pitch = 0.0f; // Set initial pitch to 0

  Serial.println("Scene initialized successfully.");
}


// Update the scene (e.g., animations, physics)
void Scene::update(float delta_time) {
  using namespace Device;
  camera.yaw += -(read_joystick_x() * 2) * delta_time;

  float3 cam_right, cam_up, cam_fwd, move_delta = { 0.0, 0.0, 0.0 };
  camera.get_basis_vectors(cam_right, cam_up, cam_fwd);

  if (read_joystick_y() < -JOYSTICK_THRESH)
    move_delta += cam_fwd; // (read_joystick_y() * 5) * delta_time;
  else if (read_joystick_y() > JOYSTICK_THRESH)
    move_delta += (cam_fwd * -1);

  /** Button Test */
  uint32_t buttons = read_buttons();
  if (buttons & BUTTON_MASK_A) {
    camera.position.y += 2 * delta_time;
  } else if (buttons & BUTTON_MASK_B) {
    camera.position.y -= 2 * delta_time;
  } else if (buttons & BUTTON_MASK_START) { // Strafe
    move_delta += cam_right;
  } if (buttons & BUTTON_MASK_SELECT) {
    move_delta += cam_right * -1;
  }

  camera.position += float3::normalize(move_delta) * 2 * delta_time;
}


// Render the scene to the display buffer
void Scene::render(uint16_t *buffer, float *depth_buffer) {
  // Get camera forward direction
  float3 cam_right, cam_up, cam_fwd;
  camera.get_basis_vectors(cam_right, cam_up, cam_fwd);

  Model model;
  model.scale = { 0.5f, 0.5f, 0.5f }; // Scale cubes to 0.5 to prevent overlap
  model.vertices = Scene::cube_vertices; // Use the predefined vertices
  model.transform.pitch = 0.0f; // Reset pitch for each cube
  model.transform.yaw = 0.0f; // Reset yaw for each cube

  // Calculate proper bounds around the camera position
  float min_x = fmaxf(0.0f, floorf(camera.position.x - MAX_DEPTH));
  float max_x = fminf((float)MAP_WIDTH, ceilf(camera.position.x + MAX_DEPTH));
  float min_y = fmaxf(0.0f, floorf(camera.position.y - MAX_DEPTH));
  float max_y = fminf((float)MAP_HEIGHT, ceilf(camera.position.y + MAX_DEPTH));
  float min_z = fmaxf(0.0f, floorf(camera.position.z - MAX_DEPTH));
  float max_z = fminf((float)MAP_DEPTH, ceilf(camera.position.z + MAX_DEPTH));

  // Reorder loops for better cache locality (y-z-x order matches memory layout _map[x][z][y])  
  for (size_t y = min_y; y < max_y; ++y) {
    for (size_t z = min_z; z < max_z; ++z) {
      for (size_t x = min_x; x < max_x; ++x) {
        // Check if block is ahead of the player
        float3 block_pos = { (float)x, (float)y, (float)z };
        float3 to_block = block_pos - camera.position;
        
        // Skip blocks behind the camera (dot product < 0)
        if (float3::dot(to_block, cam_fwd) < 0) {
          continue;
        }
        
        // Improved frustum culling - check if block is within view frustum
        float distance = float3::magnitude(to_block);
        if (distance > MAX_DEPTH) continue; // Distance culling
        
        // Project block position to screen space for quick frustum test
        float projected_x = float3::dot(to_block, cam_right);
        float projected_y = float3::dot(to_block, cam_up);
        float projected_z = float3::dot(to_block, cam_fwd);
        
        // Less aggressive frustum culling with proper block size consideration
        // FOV = 60 degrees, so half-angle = 30 degrees, tan(30°) ≈ 0.577f
        // But we need to account for block size and be less aggressive
        float frustum_half_width = projected_z * 0.8f; // Even wider to catch edge cases
        float block_radius = 1.0f; // Increased margin for safety
        
        // More conservative check - only cull if block is clearly outside view
        if (projected_z > 0.1f) { // Only do frustum culling for blocks not too close
          if (fabsf(projected_x) > frustum_half_width + block_radius || 
              fabsf(projected_y) > frustum_half_width + block_radius) {
            continue;
          }
        }
        
        block_type_t block = _map[x][z][y];

        if (block == block_type_t::AIR) {
          continue; // Skip air blocks
        }
        
        // Occlusion culling: skip blocks that are completely surrounded
        if (!is_block_visible(x, z, y)) {
          continue;
        }

        model.transform.position = { (float)x, (float)y, (float)z };

        // Assign pre-computed UV coordinates based on block type
        switch (block) {
          case block_type_t::GRASS:
            model.uvs = Scene::cube_uvs_grass;
            break;
          case block_type_t::STONE:
            model.uvs = Scene::cube_uvs_stone;
            break;
          case block_type_t::DIRT:
            model.uvs = Scene::cube_uvs_dirt;
            break;
          default:
            model.uvs = Scene::cube_uvs_stone; // Default to stone
            break;
        }

        // Render the block using the renderer
        render_model(buffer, depth_buffer, camera, model);
      }
    }
  }
}


// Generate terrain for the scene
void Scene::generate_terrain() {
  for (size_t z = 0; z < MAP_DEPTH; ++z) {
    for (size_t x = 0; x < MAP_WIDTH; ++x) {
      // Create a 2D sine wave based on both x and z coordinates
      float height_float = sin((float)x * 0.2f) * sin((float)z * 0.2f) * 4.0f + (MAP_HEIGHT / 2.0f);
      
      // Clamp to valid range before casting
      if (height_float < 0.0f) height_float = 0.0f;
      if (height_float >= MAP_HEIGHT) height_float = MAP_HEIGHT - 1;
      
      size_t y = (size_t)height_float;
      
      // Double-check bounds (defensive programming)
      if (y < MAP_HEIGHT) {
        if (y > 0 && y <= 8) // y = 0 is top of world
          _map[x][z][y] = block_type_t::GRASS;
        else
          _map[x][z][y] = block_type_t::STONE;
      }
    }
  }
}
