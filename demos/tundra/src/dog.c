#include "dog.h"

#include <stdio.h>
#include <math.h>
#include <shader-works/primitives.h>

#include "scene.h"

// magenta fragment shader declared in shaders.c
extern fragment_shader_t dog_frag;

static void rotate_dog_to_terrrain(dog_t *dog) {
  if (!dog || !dog->active) return;

  // Calculate the normal of the terrain at this position using central differences
  float epsilon = 0.1f;
  float height_left = terrainHeight(dog->position.x - epsilon, dog->position.z, g_world_config.seed);
  float height_right = terrainHeight(dog->position.x + epsilon, dog->position.z, g_world_config.seed);
  float height_down = terrainHeight(dog->position.x, dog->position.z - epsilon, g_world_config.seed);
  float height_up = terrainHeight(dog->position.x, dog->position.z + epsilon, g_world_config.seed);

  float terrain_height = terrainHeight(dog->position.x, dog->position.z, g_world_config.seed);
  float3 normal = float3_normalize(make_float3(height_left - height_right, 2.0f * epsilon, height_down - height_up));

  float3 flat_forward = make_float3(sinf(dog->target_yaw), 0, cosf(dog->target_yaw));
  float3 right = float3_normalize(float3_cross(flat_forward, normal));
  float3 forward = float3_normalize(float3_cross(normal, right));

  dog->forward = float3_normalize(float3_add(float3_scale(dog->forward, 0.8f), float3_scale(forward, 0.2f)));
  dog->right = float3_normalize(float3_add(float3_scale(dog->right, 0.8f), float3_scale(right, 0.2f)));

  dog->model.transform.yaw = atan2f(dog->forward.x, dog->forward.z);
  dog->model.transform.pitch = asinf(-dog->forward.y);

  dog->position.y = terrain_height + (dog->size / 2.0f);
  dog->model.transform.position = dog->position;
}

static void set_waypoint(dog_t *dog, float3 destination) {
  if (!dog) return;

  dog->target_destination = destination;
  dog->target_yaw = atan2f(destination.x - dog->position.x, destination.z - dog->position.z);
}

void init_dog(dog_t *dog, float3 position, float3 color, float size) {
  if (!dog) return;

  dog->position = position;
  dog->color = color;
  dog->size = size;
  dog->velocity = make_float3(0, 0, 0);
  dog->speed = 8.5f; // units per second
  dog->state = DOG_STATE_IDLE;
  dog->active = true;

  dog->forward = make_float3(0, 0, -1); // Initial forward direction
  dog->right = make_float3(1, 0, 0);    // Initial right direction
  dog->target_destination = position;    // Start with target at current position

  // Adjust position based on terrain height
  dog->position.y = terrainHeight(dog->position.x, dog->position.z, g_world_config.seed) + size / 2.0f;

  // Generate cube at the adjusted position
  generate_cube(&dog->model, dog->position, make_float3(size, size, size * 2));
  dog->model.frag_shader = &dog_frag; // magenta for visibility
  dog->model.use_textures = false;
  dog->model.vertex_shader = &default_vertex_shader;
  dog->model.disable_behind_camera_culling = true; // ensure it renders even if partially behind camera

  printf("Dog initialized at (%.2f, %.2f, %.2f), active=%d\n", dog->position.x, dog->position.y, dog->position.z, dog->active);
  printf("Dog model vertices: %zu, faces: %zu, frag_shader: %p\n", dog->model.num_vertices, dog->model.num_faces, (void*)dog->model.frag_shader);
  printf("Dog model transform: (%.2f, %.2f, %.2f)\n", dog->model.transform.position.x, dog->model.transform.position.y, dog->model.transform.position.z);
  printf("Dog model vertex_data: %p, face_normals: %p\n", (void*)dog->model.vertex_data, (void*)dog->model.face_normals);

  // pick destination atleast 200 units in the -z direction
  set_waypoint(dog, make_float3(position.x, position.y, position.z - 200.0f));
}

void update_dog(dog_t *dog, float delta_time) {
  if (!dog || !dog->active) return;

  dog->state = DOG_STATE_RUNNING; // For this demo, the dog will always run towards its target

  // move towards target destination if not already there
  float3 to_target = float3_sub(dog->target_destination, dog->position);
  float distance_to_target = float3_magnitude(to_target);
  if (distance_to_target > 0.1f) {
    float3 direction = float3_normalize(to_target);
    dog->velocity = float3_scale(direction, dog->speed);
    dog->position = float3_add(dog->position, float3_scale(dog->velocity, delta_time));
  } else {
    dog->velocity = make_float3(0, 0, 0);
    dog->state = DOG_STATE_IDLE;
  }

  // For this demo, the dog will simply rotate to align with the terrain each frame
  rotate_dog_to_terrrain(dog);
}