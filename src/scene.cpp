#include "scene.h"

#include "device.h"
#include "renderer.h"

static block_type_t _map[Scene::MAP_WIDTH][Scene::MAP_DEPTH][Scene::MAP_HEIGHT];

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

// Helper function to calculate UV coordinates for a tile
float2 get_tile_uv(int tile_id, int corner) {
  constexpr int ATLAS_WIDTH_PX = 80;
  constexpr int ATLAS_HEIGHT_PX = 24;
  constexpr int TILE_WIDTH_PX = 8;
  constexpr int TILE_HEIGHT_PX = 8;
  constexpr int TILES_PER_ROW = ATLAS_WIDTH_PX / TILE_WIDTH_PX;
  
  float tile_x_start = static_cast<float>((tile_id % TILES_PER_ROW) * TILE_WIDTH_PX);
  float tile_y_start = static_cast<float>((tile_id / TILES_PER_ROW) * TILE_HEIGHT_PX);
  
  float u_start = tile_x_start / static_cast<float>(ATLAS_WIDTH_PX);
  float v_start = tile_y_start / static_cast<float>(ATLAS_HEIGHT_PX);
  float u_end = (tile_x_start + static_cast<float>(TILE_WIDTH_PX)) / static_cast<float>(ATLAS_WIDTH_PX);
  float v_end = (tile_y_start + static_cast<float>(TILE_HEIGHT_PX)) / static_cast<float>(ATLAS_HEIGHT_PX);
  
  switch (corner) {
    case 0: return {u_start, v_end};   // top-left (flipped V)
    case 1: return {u_start, v_start}; // bottom-left (flipped V)
    case 2: return {u_end, v_start};   // bottom-right (flipped V)
    case 3: return {u_end, v_end};     // top-right (flipped V)
    default: return {u_start, v_end};
  }
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

  for (size_t x = min_x; x < max_x; ++x) {
    for (size_t z = min_z; z < max_z; ++z) {
      for (size_t y = min_y; y < max_y; ++y) {
        // Check if block is ahead of the player
        float3 block_pos = { (float)x, (float)y, (float)z };
        float3 to_block = block_pos - camera.position;
        
        // Skip blocks behind the camera (dot product < 0)
        if (float3::dot(to_block, cam_fwd) < 0) {
          continue;
        }
        
        block_type_t block = _map[x][z][y];

        if (block == block_type_t::AIR) {
          continue; // Skip air blocks
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
