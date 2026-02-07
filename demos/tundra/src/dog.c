#include "dog.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <shader-works/primitives.h>

#include "scene.h"

// --- Steering Constants ---
#define MAX_FORCE 18.0f
#define WANDER_DISTANCE 2.0f
#define WANDER_RADIUS 1.5f
#define WANDER_JITTER 5.0f
#define ARRIVAL_RADIUS 20.0f

extern fragment_shader_t dog_frag;

// Helper: Seek force with Arrival (slow down at target)
static float3 steer_seek(dog_t *dog, float3 target) {
  float3 desired = float3_sub(target, dog->position);
  float distance = float3_magnitude(desired);
  
  if (distance < 0.001f) return make_float3(0, 0, 0);
  
  desired = float3_normalize(desired);
  
  // Arrival behavior: scale speed down as we get closer
  if (distance < ARRIVAL_RADIUS) {
    float speed = (distance / ARRIVAL_RADIUS) * dog->speed;
    desired = float3_scale(desired, speed);
  } else {
    desired = float3_scale(desired, dog->speed);
  }
  
  float3 steer = float3_sub(desired, dog->velocity);
  return steer;
}

// Helper: Perlin Wander force
static float3 steer_wander(dog_t *dog, float delta_time) {
  static float wander_timer = 0.0f;
  wander_timer += delta_time * WANDER_JITTER;
  
  // Circle projected in front
  float3 circle_center = float3_scale(dog->forward, WANDER_DISTANCE);
  
  // Use world seed for consistent but "organic" noise
  float noise = terrainHeight(wander_timer, 0.0f, g_world_config.seed); 
  float angle = noise * 2.0f * 3.14159f;
  
  float3 displacement = make_float3(cosf(angle), 0, sinf(angle));
  displacement = float3_scale(displacement, WANDER_RADIUS);
  
  return float3_add(circle_center, displacement);
}

static void rotate_dog_to_terrrain(dog_t *dog) {
  if (!dog || !dog->active) return;

  float epsilon = 0.1f;
  float height_left = terrainHeight(dog->position.x - epsilon, dog->position.z, g_world_config.seed);
  float height_right = terrainHeight(dog->position.x + epsilon, dog->position.z, g_world_config.seed);
  float height_down = terrainHeight(dog->position.x, dog->position.z - epsilon, g_world_config.seed);
  float height_up = terrainHeight(dog->position.x, dog->position.z + epsilon, g_world_config.seed);

  float terrain_height = terrainHeight(dog->position.x, dog->position.z, g_world_config.seed);
  float3 normal = float3_normalize(make_float3(height_left - height_right, 2.0f * epsilon, height_down - height_up));

  // Use current velocity to influence target yaw so he looks where he's steering
  if (float3_magnitude(dog->velocity) > 0.1f) {
    dog->target_yaws[dog->waypoint_idx] = atan2f(dog->velocity.x, dog->velocity.z);
  }

  float3 flat_forward = make_float3(sinf(dog->target_yaws[dog->waypoint_idx]), 0, cosf(dog->target_yaws[dog->waypoint_idx]));
  float3 right = float3_normalize(float3_cross(flat_forward, normal));
  float3 forward = float3_normalize(float3_cross(normal, right));

  dog->forward = float3_normalize(float3_add(float3_scale(dog->forward, 0.8f), float3_scale(forward, 0.2f)));
  dog->right = float3_normalize(float3_add(float3_scale(dog->right, 0.8f), float3_scale(right, 0.2f)));

  dog->model.transform.yaw = atan2f(dog->forward.x, dog->forward.z);
  dog->model.transform.pitch = asinf(-dog->forward.y);

  dog->position.y = terrain_height + (dog->size / 2.0f);
  dog->model.transform.position = dog->position;
}

static void generate_waypoints(dog_t *dog) {
  if (!dog) return;
  
  if (!dog->target_destinations) {
    dog->target_destinations = calloc(MAX_DOG_WAYPOINTS, sizeof(float3));
    dog->target_yaws = calloc(MAX_DOG_WAYPOINTS, sizeof(float));
  }
  
  float3 current_search_origin = dog->position;
  
  for (unsigned int i = 0; i < MAX_DOG_WAYPOINTS; i++) {
    float3 best_point = current_search_origin;
    float max_score = -1e6f;
    
    // Search in a "fan" in the -z direction
    // Samples at different angles and distances
    for (float angle = -0.8f; angle <= 0.8f; angle += 0.4f) { // ~40 degree spread
      for (float dist = 10.0f; dist <= 25.0f; dist += 5.0f) {
        
        float3 sample_pos;
        sample_pos.x = current_search_origin.x + sinf(angle) * dist;
        sample_pos.z = current_search_origin.z - cosf(angle) * dist; // Force -z movement
        sample_pos.y = terrainHeight(sample_pos.x, sample_pos.z, g_world_config.seed);
        
        // The "Score" combines height (exploration) and -z progress (consistency)
        // We give a bonus for being higher, but also for being further 'north' (-z)
        float z_progress = (current_search_origin.z - sample_pos.z); 
        float score = (sample_pos.y * 3.0f) + z_progress; 
        
        if (score > max_score) {
          max_score = score;
          best_point = sample_pos;
        }
      }
    }
    
    // best_point.x += cosf(i * 0.5f) * 35.0f; // Add some variation to prevent perfect grid-like paths
    dog->target_destinations[i] = best_point;
    // Calculate yaw relative to the previous point to keep the orientation correct
    float3 diff = float3_sub(best_point, current_search_origin);
    dog->target_yaws[i] = atan2f(diff.x, diff.z);
    
    current_search_origin = best_point; // Move origin for next waypoint
    
    printf("Waypoint %u: (%.2f, %.2f, %.2f) | Yaw: %.2f\n", 
      i, best_point.x, best_point.y, best_point.z, dog->target_yaws[i]);
    }
  }

float get_target_distance(dog_t *dog) {
  if (!dog || !dog->active) return 0.0f;

  float3 target = dog->target_destinations[dog->waypoint_idx];
  return float3_magnitude(float3_sub(target, dog->position));
}

static void next_waypoint(dog_t *dog) {
  if (!dog) return;
  if (++dog->waypoint_idx >= MAX_DOG_WAYPOINTS) {
    dog->waypoint_idx = MAX_DOG_WAYPOINTS; // Loop back to the first waypoint
  }

  printf("Next waypoint: %u, Target: (%.2f, %.2f, %.2f), Yaw: %.2f\n", 
          dog->waypoint_idx, 
          dog->target_destinations[dog->waypoint_idx].x, 
          dog->target_destinations[dog->waypoint_idx].y, 
          dog->target_destinations[dog->waypoint_idx].z,
          dog->target_yaws[dog->waypoint_idx]);
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

  dog->waypoint_idx = 0;
  generate_waypoints(dog);

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
}

void update_dog(dog_t *dog, float player_distance, float delta_time) {
  if (!dog || !dog->active) return;

  float3 seek = steer_seek(dog, dog->target_destinations[dog->waypoint_idx]);
  float3 wander = steer_wander(dog, delta_time);
  float3 total_steer = float3_add(float3_scale(seek, 1.2f), float3_scale(wander, 0.5f));
  
  // Clamp force
  if (float3_magnitude(total_steer) > MAX_FORCE) {
    total_steer = float3_scale(float3_normalize(total_steer), MAX_FORCE);
  }

  // Assuming mass = 1.0f for simplicity
  dog->velocity = float3_add(dog->velocity, float3_scale(total_steer, delta_time));
  
  // Cap velocity to max speed
  if (float3_magnitude(dog->velocity) > dog->speed) {
    dog->velocity = float3_scale(float3_normalize(dog->velocity), dog->speed);
  }

  dog->position = float3_add(dog->position, float3_scale(dog->velocity, delta_time));

  if (get_target_distance(dog) < 4.0f && player_distance <= 10.0f) {
    next_waypoint(dog);
  }

  rotate_dog_to_terrrain(dog);
}