#include "world.h"

#include <stdlib.h>

extern fragment_shader_t ground_frag;

void init_world(world_t *world) {
  world->sectors = NULL;
  world->tail = NULL;
  world->num_sectors = 0;
  world->lights = calloc(MAX_LIGHTS, sizeof(light_t));
  world->num_lights = 0;
}

void delete_world(world_t *world) {
  sector_t *sector = world->sectors;
  while (sector) {
    sector_t *next = sector->next;
    free(sector);
    sector = next;
  }

  world->sectors = NULL;
  world->tail = NULL;
  world->num_sectors = 0;

  if (world->lights) {
    free(world->lights);
    world->num_lights = 0;
  }
}

void add_sector(world_t *world, int x, int y, int w, int d, int ceiling_height) {
  sector_t *new_sector = malloc(sizeof(sector_t));
  new_sector->x = x;
  new_sector->y = y;
  new_sector->w = w;
  new_sector->d = d;
  new_sector->ceiling_height = ceiling_height;
  new_sector->next = NULL;
  new_sector->prev = world->tail;
  new_sector->num_walls = 0;

  new_sector->floor = (model_t){0};
  new_sector->ceiling = (model_t){0};

  // generate floor plane for sector
  generate_plane_with_norm(&new_sector->floor, (float2){w, d}, (float2){1.0f, 1.0f}, (float3){x + w * 0.5f, 0.0f, y + d * 0.5f}, (float3){0.0f, -1.0f, 0.0f});
  new_sector->floor.vertex_shader = &default_vertex_shader;
  new_sector->floor.frag_shader = &ground_frag;

  // generate ceiling plane for sector
  generate_plane_with_norm(&new_sector->ceiling, (float2){w, d}, (float2){1.0f, 1.0f}, (float3){x + w * 0.5f, (float)ceiling_height, y + d * 0.5f}, (float3){0.0f, 1.0f, 0.0f});
  new_sector->ceiling.vertex_shader = &default_vertex_shader;
  new_sector->ceiling.frag_shader = &ground_frag;

  // add one light in the center of the sector
  if (world->num_lights < MAX_LIGHTS) {
    world->lights[world->num_lights].position = (float3){ x + w * 0.5f, ceiling_height - 1.0f, y + d * 0.5f };
    world->lights[world->num_lights].color = rgb_to_u32(255, 255, 255);
    world->lights[world->num_lights].is_directional = false;
    world->num_lights++;
  }

  if (world->tail) {
    world->tail->next = new_sector;
  } else {
    world->sectors = new_sector; // Set head if it's the first one
  }

  world->tail = new_sector;
  world->num_sectors++;
}

void add_walls(world_t *world) {
  sector_t *sector = world->sectors;
  while (sector) {
    sector->num_walls = 4;
    sector->walls = calloc(sector->num_walls, sizeof(model_t));

    for (int i = 0; i < 4; i++) {
      model_t *wall = &sector->walls[i];
      float3 pos = {0};
      float2 wall_size = {0};
      float wall_yaw = 0.0f;

      // Determine wall geometry based on index (N, S, E, W)
      if (i == 0) { // North (Far)
        wall_size = (float2){ (float)sector->w, (float)sector->ceiling_height };
        pos = (float3){ sector->x + sector->w * 0.5f, sector->ceiling_height * 0.5f, (float)sector->x + sector->d };
        wall_yaw = 0.0f;
      } else if (i == 1) { // South (Near)
        wall_size = (float2){ (float)sector->w, (float)sector->ceiling_height };
        pos = (float3){ sector->x + sector->w * 0.5f, sector->ceiling_height * 0.5f, (float)sector->x };
        wall_yaw = PI;
      } else if (i == 2) { // East (Right)
        wall_size = (float2){ (float)sector->d, (float)sector->ceiling_height };
        pos = (float3){ (float)sector->x + sector->w, sector->ceiling_height * 0.5f, sector->x + sector->d * 0.5f };
        wall_yaw = PI / 2.0f;
      } else if (i == 3) { // West (Left)
        wall_size = (float2){ (float)sector->d, (float)sector->ceiling_height };
        pos = (float3){ (float)sector->x, sector->ceiling_height * 0.5f, sector->x + sector->d * 0.5f };
        wall_yaw = -PI / 2.0f;
      }

      generate_plane_with_norm(wall, wall_size, (float2){1.0f, 1.0f}, (float3){0,0,0}, (float3){0, 1, 0});

      wall->transform.pitch = PI / 2.0f; // Stand it up
      wall->transform.yaw = wall_yaw;    // Face it toward the player
      wall->transform.position = pos;    // Move it to the edge

      wall->vertex_shader = &default_vertex_shader;
      wall->frag_shader = &ground_frag;
    }

    sector = sector->next;
  }
}

void render_world(renderer_t *renderer, world_t *world, transform_t *camera) {
  sector_t *sector = world->sectors;
  while (sector) {
    render_model(renderer, camera, &sector->floor, world->lights, world->num_lights);
    render_model(renderer, camera, &sector->ceiling, world->lights, world->num_lights);

    for (usize i = 0; i < sector->num_walls && sector->num_walls > 0; i++) {
      if (sector->walls[i].num_vertices > 0) {
        render_model(renderer, camera, &sector->walls[i], world->lights, world->num_lights);
      }
    }
    sector = sector->next;
  }
}
