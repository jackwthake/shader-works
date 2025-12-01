#include "scene.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include <SDL3/SDL.h>
#include <shader-works/renderer.h>
#include <shader-works/maths.h>

#include "common/noise.h"

extern fragment_shader_t ground_shadow_frag;

// Function to set scene data for shadow calculations (defined in shaders.c)
extern void set_shadow_scene(scene_t *scene);

extern void generate_ground_plane(model_t *, float2, float2, float3);  // in proc_gen.c

/**
 * returns Level of detail based off the passed distance
 * NOTE: the passed distance must be squared, this is for speed
 *
 * DO NOT TWEAK THIS IS VERY WELL TUNED
 */
static int get_lod_from_dist(int dist_sq) {
  float chunk_size_sq = g_world_config.chunk_size * g_world_config.chunk_size;

  if (g_world_config.use_high_graphics) { /* High quality */
    if (dist_sq >= 0 && dist_sq <= chunk_size_sq) return 2;
    else if (dist_sq <= (2 * 2) * chunk_size_sq) return 3;
    else if (dist_sq <= (2.5f * 2.5f) * chunk_size_sq) return 4;
    else if (dist_sq <= (3 * 3) * chunk_size_sq) return 5;
    else if (dist_sq <= (3.5f * 3.5f) * chunk_size_sq) return 9;
    else if (dist_sq <= (4 * 4) * chunk_size_sq) return 10;
    else if (dist_sq <= (4.5f * 4.5f) * chunk_size_sq) return 15;
  } else { /* Low quality*/
    if (dist_sq >= 0 && dist_sq <= chunk_size_sq) return 4;
    else if (dist_sq <= (2.5f * 2.5f) * chunk_size_sq) return 5;
    else if (dist_sq <= (3 * 3) * chunk_size_sq) return 8;
    else if (dist_sq <= (3.5f * 3.5f) * chunk_size_sq) return 9;
    else if (dist_sq <= (4 * 4) * chunk_size_sq) return 12;
    else if (dist_sq <= (4.5f * 4.5f) * chunk_size_sq) return 15;
  }

  return 20;
}

static void generate_chunk(chunk_t *chunk, int chunk_x, int chunk_z, float player_x, float player_z) {
  if (chunk == NULL) return;

  chunk->x = chunk_x;
  chunk->z = chunk_z;
  chunk->ground_plane = (model_t){0};

  float world_x = chunk_x * g_world_config.chunk_size + g_world_config.half_chunk_size;
  float world_z = chunk_z * g_world_config.chunk_size + g_world_config.half_chunk_size;
  float corner_x = world_x;
  float corner_z = world_z;

  int dx = player_x - world_x;
  int dz = player_z - world_z;

  int dist_sq = (dx * dx) + (dz * dz);

  chunk->lod = get_lod_from_dist(dist_sq);

  generate_ground_plane(&chunk->ground_plane, make_float2(g_world_config.chunk_size, g_world_config.chunk_size), make_float2(chunk->lod, chunk->lod), make_float3(corner_x + g_world_config.half_chunk_size, 0, corner_z + g_world_config.half_chunk_size));
  chunk->ground_plane.frag_shader = &ground_shadow_frag;

  // generate points of interest
  if (ridgeNoise(chunk_x, chunk_z, g_world_config.seed) > 0.95f) {
    chunk->static_objs = calloc(1, sizeof(model_t));
    chunk->num_static_objs = 1;

    float obj_x = (chunk->x * g_world_config.chunk_size) + ((hash2(chunk->x, chunk->z, g_world_config.seed) + 1) * g_world_config.chunk_size);
    float obj_z = (chunk->z * g_world_config.chunk_size) + ((hash2(chunk->x, chunk->z, g_world_config.seed + 1) + 1) * g_world_config.chunk_size);
    float3 pos = {obj_x, get_interpolated_terrain_height(obj_x, obj_z) + 3, obj_z};

    generate_cube(chunk->static_objs, pos, make_float3(8.f, 8.f, 8.f));
    chunk->static_objs->use_textures = false;
  } else {
    chunk->num_static_objs = 0;
  }
}

static void validate_lod_level(scene_t *scene, chunk_map_node_t *node) {
  chunk_t chunk = node->chunk;
  float chunk_world_x = (float)(chunk.x * g_world_config.chunk_size) + g_world_config.half_chunk_size;
  float chunk_world_z = (float)(chunk.z * g_world_config.chunk_size) + g_world_config.half_chunk_size;
  float dx = scene->camera_pos.position.x - chunk_world_x;
  float dz = scene->camera_pos.position.z - chunk_world_z;

  float dist_sq = (dx * dx) + (dz * dz);
  if (get_lod_from_dist(dist_sq) != chunk.lod) {
    int chunk_x = chunk.x, chunk_z = chunk.z;
    remove_chunk(&scene->chunk_map, chunk_x, chunk_z);

    chunk = (chunk_t){0};

    generate_chunk(&chunk, chunk_x, chunk_z, scene->camera_pos.position.x, scene->camera_pos.position.z);
    insert_chunk(&scene->chunk_map, &chunk);
  }
}

static usize render_chunk(renderer_t *state, chunk_t *chunk, transform_t *camera, light_t *lights, const usize num_lights, scene_t *scene) {
  (void)scene;
  usize triangles_rendered = 0;
  if (chunk->ground_plane.vertex_data != NULL && chunk->ground_plane.num_vertices > 0) {
    triangles_rendered += render_model(state, camera, &chunk->ground_plane, lights, num_lights);
  }

  for (usize i = 0; i < chunk->num_static_objs; ++i) {
    if (chunk->static_objs[i].vertex_data != NULL && chunk->static_objs[i].num_vertices > 0) {
      triangles_rendered += render_model(state, camera, &chunk->static_objs[i], lights, num_lights);
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
    .min_pitch = -PI/2,
    .max_pitch = PI/2,
    .ground_height = 3.0f,
    .camera_height_offset = 3.f,
    .last_frame_time = SDL_GetPerformanceCounter(),
    .distance_walked = 0.f
  };

  scene->camera_pos = (transform_t){ 0 };
  scene->fog_start = 0.75;

  init_chunk_map(&scene->chunk_map, CHUNK_MAP_NUM_BUCKETS);
}

bool cull_chunk(chunk_t *chunk, void *param, usize num_params) {
  (void)num_params;
  if (!chunk || !param) return true;

  transform_t *player = (transform_t*)param;

  // Calculate player's chunk coordinates (handle negative coordinates properly)
  int player_chunk_x = (int)floorf(player->position.x / g_world_config.chunk_size);
  int player_chunk_z = (int)floorf(player->position.z / g_world_config.chunk_size);

  int dx = chunk->x - player_chunk_x;
  int dz = chunk->z - player_chunk_z;

  float dist = sqrtf((dx * dx) + (dz * dz));

  if (dist <= g_world_config.chunk_load_radius)
    return false;

  return true;
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
        generate_chunk(&new_chunk, chunk_x, chunk_z, scene->camera_pos.position.x, scene->camera_pos.position.z);
        insert_chunk(&scene->chunk_map, &new_chunk);
      } else {
        validate_lod_level(scene, chunk_lookup(&scene->chunk_map, chunk_x, chunk_z));
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

usize render_loaded_chunks(renderer_t *restrict state, scene_t *restrict scene, light_t *restrict lights, const usize num_lights) {
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
      if (dot > (g_world_config.chunk_size * 2)) {
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
