#include "dog.h"

#include <stdio.h>
#include <shader-works/primitives.h>

#include "scene.h"

// magenta fragment shader declared in shaders.c
extern fragment_shader_t dog_frag;

void init_dog(dog_t *dog, float3 position, float3 color, float size) {
  if (!dog) return;

  dog->position = position;
  dog->color = color;
  dog->size = size;
  dog->velocity = make_float3(0, 0, 0);
  dog->state = DOG_STATE_IDLE;
  dog->active = true;

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

void update_dog(dog_t *dog, float delta_time) {
  (void)dog; (void)delta_time;
}