#include "scene.h"

#include "device.h"
#include "renderer.h"


float3 Scene::cube_vertices[36] = {
  {-1.000000, -1.000000, -1.000000}, {-1.000000, -1.000000, 1.000000}, {1.000000, -1.000000, 1.000000}, 
  {-1.000000, -1.000000, -1.000000}, {1.000000, -1.000000, 1.000000}, {1.000000, -1.000000, -1.000000}, 
  {1.000000, 1.000000, -1.000000}, {1.000000, 1.000000, 1.000000}, {-1.000000, 1.000000, 1.000000}, 
  {1.000000, 1.000000, -1.000000}, {-1.000000, 1.000000, 1.000000}, {-1.000000, 1.000000, -1.000000}, 
  {1.000000, -1.000000, 1.000000}, {1.000000, 1.000000, 1.000000}, {-1.000000, 1.000000, 1.000000}, 
  {1.000000, -1.000000, 1.000000}, {-1.000000, 1.000000, 1.000000}, {-1.000000, -1.000000, 1.000000}, 
  {-1.000000, -1.000000, -1.000000}, {-1.000000, 1.000000, -1.000000}, {1.000000, 1.000000, -1.000000}, 
  {-1.000000, -1.000000, -1.000000}, {1.000000, 1.000000, -1.000000}, {1.000000, -1.000000, -1.000000}, 
  {1.000000, -1.000000, -1.000000}, {1.000000, 1.000000, -1.000000}, {1.000000, 1.000000, 1.000000}, 
  {1.000000, -1.000000, -1.000000}, {1.000000, 1.000000, 1.000000}, {1.000000, -1.000000, 1.000000}, 
  {-1.000000, -1.000000, 1.000000}, {-1.000000, 1.000000, 1.000000}, {-1.000000, 1.000000, -1.000000}, 
  {-1.000000, -1.000000, 1.000000}, {-1.000000, 1.000000, -1.000000}, {-1.000000, -1.000000, -1.000000}
};


// Initialize the scene, load resources, etc.
void Scene::init() {
  for (size_t z = 0; z < MAP_DEPTH; ++z) {
    for (size_t y = 0; y < MAP_HEIGHT; ++y) {
      for (size_t x = 0; x < MAP_WIDTH; ++x) {
        map[x][y][z] = block_type_t::AIR; // Initialize all blocks to AIR
      }
    }
  }

  generate_terrain();

  camera.position = { 4.0f, 2.0f, 4.0f }; // Set initial camera position
  camera.yaw = 0.0f; // Set initial yaw to 0
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
  for (size_t z = 0; z < MAP_DEPTH; ++z) {
    for (size_t y = 0; y < MAP_HEIGHT; ++y) {
      for (size_t x = 0; x < MAP_WIDTH; ++x) {
        block_type_t block = map[x][y][z];

        if (block == block_type_t::AIR) {
          continue; // Skip air blocks
        }

        Model model;
        model.scale = { 0.5f, 0.5f, 0.5f }; // Scale cubes to 0.5 to prevent overlap
        model.vertices = Scene::cube_vertices; // Use the predefined vertices

        // Set the position of this cube in the world
        model.transform.position = { (float)x, (float)y, (float)z };
        model.transform.yaw = 0;
        model.transform.pitch = 0;

        compute_uv_coords(model.uvs, 2); // top face
        compute_uv_coords(model.uvs, 0); // bottom face
        compute_uv_coords(model.uvs, 1); // side face
        compute_uv_coords(model.uvs, 1); // side face
        compute_uv_coords(model.uvs, 1); // side face
        compute_uv_coords(model.uvs, 1); // side face

        // Render the block using the renderer
        render_model(buffer, depth_buffer, camera, model);
      }
    }
  }
}


// Generate terrain for the scene
void Scene::generate_terrain() {
  // generate a flat plane at y=0 with a simple pattern
  float t = 0;
  for (size_t z = 0; z < MAP_DEPTH; ++z) {
    for (size_t x = 0; x < MAP_WIDTH; ++x) {
      size_t y = (size_t)(sin(t) * 4 + 4); // Offset by 4 to ensure positive values
      if (y >= 0 && y < MAP_HEIGHT) { // Bounds check
        map[x][y][z] = block_type_t::GRASS; // Grass blocks
      }
    }

    t += 0.1f; // Increment t for the next row
  }
}
