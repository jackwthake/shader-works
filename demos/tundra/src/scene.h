#ifndef SCENE_H
#define SCENE_H

#include <shader-works/renderer.h>
#include <shader-works/primitives.h>
#include <shader-works/maths.h>

#include "util/chunk_map.h"
#include "util/config.h"

typedef struct {
  float move_speed;
  float mouse_sensitivity;
  float min_pitch, max_pitch;
  float ground_height;
  float camera_height_offset;
  float delta_time;
  uint64_t last_frame_time;
} fps_controller_t;

typedef struct scene_t {
  transform_t camera_pos;
  fps_controller_t controller;

  chunk_map_t chunk_map;
  light_t sun;
} scene_t;

// Implementation found in proc_gen.c
extern float map_range(float value, float old_min, float old_max, float new_min, float new_max);

extern float lerp(float a, float b, float t);
extern float hash2(int x, int y, int seed);

extern float noise2D(float x, float y, int seed);
extern float ridgeNoise(float x, float y, int seed);
extern float terrainHeight(float x, float y, int seed);
extern float get_interpolated_terrain_height(float x, float z);

// Implementation found in scene.c
void init_scene(scene_t *scene, usize max_loaded_chunks);
void update_loaded_chunks(scene_t *scene);
usize render_loaded_chunks(renderer_t *state, scene_t *scene, light_t *lights, const usize num_lights);

// Implementation found in shaders.c
void update_quads(float3 player_pos, transform_t *camera_transform);
usize render_quads(renderer_t *renderer, transform_t *camera, light_t *lights, usize num_lights);


#endif // SCENE_H