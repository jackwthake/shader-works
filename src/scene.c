#include "scene.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include <SDL3/SDL.h>
#include <shader-works/renderer.h>
#include <shader-works/maths.h>

extern fragment_shader_t ground_shadow_frag;
extern fragment_shader_t tree_frag;

// Function to set scene data for shadow calculations (defined in shaders.c)
extern void set_shadow_scene(scene_t *scene);

extern int generate_tree(model_t *, float, float, float3, float, usize, const usize, const usize, const usize); // in proc_gen.c
extern void generate_ground_plane(model_t *, float2, float2, float3);                                           // in proc_gen.c

static void generate_chunk(chunk_t *chunk, int chunk_x, int chunk_z) {
  if (chunk == NULL) return;

  chunk->x = chunk_x;
  chunk->z = chunk_z;
  chunk->ground_plane = (model_t){0};

  float world_x = chunk_x * g_world_config.chunk_size;
  float world_z = chunk_z * g_world_config.chunk_size;
  float corner_x = world_x;
  float corner_z = world_z;

  generate_ground_plane(&chunk->ground_plane, make_float2(g_world_config.chunk_size, g_world_config.chunk_size), make_float2(1.0f, 1.0f), make_float3(corner_x + g_world_config.half_chunk_size, 0, corner_z + g_world_config.half_chunk_size));
  chunk->ground_plane.frag_shader = &ground_shadow_frag;

  chunk->num_trees = map_range(hash2(chunk_x, chunk_z, g_world_config.seed), -1.0f, 1.0f, 0, 7);
  chunk->trees = calloc(chunk->num_trees, sizeof(model_t));

  for (usize i = 0; i < chunk->num_trees; ++i) {
    float tree_x = map_range(hash2(chunk_x * 100 + i, chunk_z * 100 + i * 3, g_world_config.seed), -1.0f, 1.0f, world_x + 2, world_x + g_world_config.chunk_size - 2);
    float tree_z = map_range(hash2(chunk_z * 100 + i * 7, chunk_x * 100 + i * 5, g_world_config.seed), -1.0f, 1.0f, world_z + 2, world_z + g_world_config.chunk_size - 2);
    float tree_y = terrainHeight(tree_x, tree_z, g_world_config.seed) - 0.5f;

    if (tree_y <= 0.1f) {
      continue;
    }

    float3 tree_pos = make_float3(tree_x, tree_y, tree_z);

    float chunk_center_x = world_x + g_world_config.half_chunk_size;
    float chunk_center_z = world_z + g_world_config.half_chunk_size;
    float distance_to_chunk = sqrtf((chunk_center_x * chunk_center_x) + (chunk_center_z * chunk_center_z));
    float lod_factor = 1.0f;
    usize segments = 5;
    if (distance_to_chunk > 100.0f) {
      lod_factor = 0.5f;
      segments = 4;
    } else if (distance_to_chunk > 50.0f) {
      lod_factor = 0.7f;
      segments = 4;
    }
    float base_radius = map_range(hash2(tree_pos.x, tree_pos.z, g_world_config.seed), -1.0f, 1.0f, 0.4f, 0.55f);
    float base_angle = map_range(hash2(tree_pos.x, tree_pos.z, g_world_config.seed), -1.0f, 1.0f, 0.0f, 2.0f * PI);

    float branch_chance = map_range(hash2(tree_pos.x, tree_pos.z, g_world_config.seed), -1.0f, 1.0f, 0.85 * lod_factor, 0.95 * lod_factor);
    usize max_branches = (usize)map_range(hash2(tree_pos.x, tree_pos.z, g_world_config.seed), -1.0f, 1.0f, 4.f * lod_factor, 6.f * lod_factor);
    usize num_levels = (usize)map_range(hash2(tree_pos.x, tree_pos.z, g_world_config.seed), -1.0f, 1.0f, 4.f * lod_factor, 5.f * lod_factor);
    if (max_branches < 3) max_branches = 3;
    if (num_levels < 4) num_levels = 4;
    if (branch_chance < 0.75) branch_chance = 0.75;

    chunk->trees[i].frag_shader = &tree_frag;

    generate_tree(&(chunk->trees[i]), base_radius, base_angle, tree_pos, branch_chance, 0, max_branches, num_levels, segments);
  }
}

static usize render_chunk(renderer_t *state, chunk_t *chunk, transform_t *camera, light_t *lights, const usize num_lights, scene_t *scene) {
  (void)scene;
  usize triangles_rendered = 0;
  if (chunk->ground_plane.vertex_data != NULL && chunk->ground_plane.num_vertices > 0) {
    triangles_rendered += render_model(state, camera, &chunk->ground_plane, lights, num_lights);
  }

  for (usize i = 0; i < chunk->num_trees; ++i) {
    if (chunk->trees[i].vertex_data != NULL && chunk->trees[i].num_vertices > 0) {
      triangles_rendered += render_model(state, camera, &chunk->trees[i], lights, num_lights);
    }
  }

  return triangles_rendered;
}

void init_scene(scene_t *scene, usize max_loaded_chunks) {
  (void)max_loaded_chunks;
  if (!scene) return;
  
  scene->controller = (fps_controller_t){
    .move_speed = 8.0f,
    .mouse_sensitivity = 0.0015f,
    .min_pitch = -PI/3,
    .max_pitch = PI/3,
    .ground_height = 2.0f,
    .camera_height_offset = 6.0f,
    .last_frame_time = SDL_GetPerformanceCounter()
  };
  
  scene->camera_pos = (transform_t){ 0 };
  
  init_chunk_map(&scene->chunk_map, CHUNK_MAP_NUM_BUCKETS);
}

bool cull_chunk(chunk_t *chunk, void *param, usize num_params) {
  (void)num_params;
  if (!chunk || !param) return true;
  
  transform_t *player = (transform_t*)param;
  
  // Calculate player's chunk coordinates (handle negative coordinates properly)
  int player_chunk_x = (int)floorf(player->position.x / g_world_config.chunk_size);
  int player_chunk_z = (int)floorf(player->position.z / g_world_config.chunk_size);
  
  // Check if chunk is within the 2x3 grid pattern
  int dx = chunk->x - player_chunk_x;
  int dz = chunk->z - player_chunk_z;
  
  float3 right, up, fwd;
  transform_get_inverse_basis_vectors(player, &right, &up, &fwd);
  
  // Check if chunk matches the xxx/xPx pattern
  float chunk_offsets[][2] = {
    {0, 0}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}
  };
  
  int num_chunks = sizeof(chunk_offsets) / sizeof(chunk_offsets[0]);
  
  for (int i = 0; i < num_chunks; i++) {
    float rel_x = chunk_offsets[i][0];
    float rel_z = chunk_offsets[i][1];
    int world_dx = (int)roundf(rel_x * right.x + rel_z * fwd.x);
    int world_dz = (int)roundf(rel_x * right.z + rel_z * fwd.z);
    
    if (dx == world_dx && dz == world_dz) {
      return false; // chunk is within the pattern
    }
  }
  
  return true; // cull chunk outside the 2x3 grid
}

void update_loaded_chunks(scene_t *scene) {
  remove_chunk_if(&scene->chunk_map, cull_chunk, &scene->camera_pos, 1);

  int player_chunk_x = (int)floorf(scene->camera_pos.position.x / g_world_config.chunk_size);
  int player_chunk_z = (int)floorf(scene->camera_pos.position.z / g_world_config.chunk_size);

  for (int dx = -g_world_config.chunk_load_radius; dx <= g_world_config.chunk_load_radius; dx++) {
    for (int dz = -g_world_config.chunk_load_radius; dz <= g_world_config.chunk_load_radius; dz++) {
      int chunk_x = player_chunk_x + dx;
      int chunk_z = player_chunk_z + dz;

      if (!is_chunk_loaded(&scene->chunk_map, chunk_x, chunk_z)) {
        chunk_t new_chunk = {0};
        generate_chunk(&new_chunk, chunk_x, chunk_z);
        insert_chunk(&scene->chunk_map, &new_chunk);
      }
    }
  }
}

typedef struct {
  chunk_t *chunk;
  float distance;
} chunk_distance_t;
static int compare_chunks_by_distance(const void *a, const void *b) {
  const chunk_distance_t *chunk_a = (const chunk_distance_t*)a;
  const chunk_distance_t *chunk_b = (const chunk_distance_t*)b;
  
  if (chunk_a->distance < chunk_b->distance) return -1;
  if (chunk_a->distance > chunk_b->distance) return 1;
  return 0;
}

usize render_loaded_chunks(renderer_t *state, scene_t *scene, light_t *lights, const usize num_lights) {
  set_shadow_scene(scene);

  chunk_t **chunks = calloc(g_world_config.max_chunks, sizeof(chunk_t*));
  usize chunk_count = 0;
  usize total_triangles_rendered = 0;

  get_all_chunks(&scene->chunk_map, chunks, &chunk_count);

  if (chunk_count == 0) {
    free(chunks);
    return 0;
  }

  chunk_distance_t *sorted_chunks = calloc(chunk_count, sizeof(chunk_distance_t));

  float2 camera_pos = make_float2(scene->camera_pos.position.x, scene->camera_pos.position.z);
  for (usize i = 0; i < chunk_count; i++) {
    if (chunks[i]) {
      float2 chunk_center = make_float2(
        chunks[i]->x * g_world_config.chunk_size + g_world_config.half_chunk_size,
        chunks[i]->z * g_world_config.chunk_size + g_world_config.half_chunk_size
      );
      
      float distance = float2_magnitude(float2_sub(camera_pos, chunk_center));
      
      sorted_chunks[i].chunk = chunks[i];
      sorted_chunks[i].distance = distance;
    }
  }

  // Get camera forward vector (player looks in -Z direction)
  float3 right, up, forward;
  transform_get_basis_vectors(&scene->camera_pos, &right, &up, &forward);

  qsort(sorted_chunks, chunk_count, sizeof(chunk_distance_t), compare_chunks_by_distance);
  for (usize i = 0; i < chunk_count; i++) {
    if (sorted_chunks[i].chunk) {
      // Calculate vector from camera to chunk center
      float3 chunk_center = make_float3(
        sorted_chunks[i].chunk->x * g_world_config.chunk_size + g_world_config.half_chunk_size,
        0,
        sorted_chunks[i].chunk->z * g_world_config.chunk_size + g_world_config.half_chunk_size
      );
      float3 to_chunk = float3_sub(chunk_center, scene->camera_pos.position);

      // Check if we're in overhead mode (pitch near -PI/2)
      bool is_overhead = fabsf(scene->camera_pos.pitch + PI / 2) < 0.1f;

      // Dot product with forward vector (negative because forward is -Z)
      // If dot < 0, chunk is behind the camera (except in overhead mode)
      // Add chunk_size buffer to account for chunk size and avoid culling visible chunks
      float dot = is_overhead ? 1.0f : float3_dot(to_chunk, forward);
      if (dot < -(g_world_config.chunk_size * 2)) {
        // Chunk is fully behind the player, skip rendering
        continue;
      }

      total_triangles_rendered += render_chunk(state, sorted_chunks[i].chunk, &scene->camera_pos, lights, num_lights, scene);
    }
  }

  free(chunks);
  free(sorted_chunks);

  return total_triangles_rendered;
}