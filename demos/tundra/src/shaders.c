#include <shader-works/shaders.h>

#include <SDL3/SDL.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "scene.h"

#include "common/chunk_map.h"
#include "common/noise.h"

u32 rgb_to_u32(u8 r, u8 g, u8 b) {
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);
  return SDL_MapRGBA(format, NULL, r, g, b, 255);
}

void u32_to_rgb(u32 color, u8 *r, u8 *g, u8 *b) {
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);
  SDL_GetRGB(color, format, NULL, r, g, b);
}

u32 tree_frag_func(u32 input, fragment_context_t *ctx, void *args, usize argc) {
  (void)input; (void)args; (void)argc;

  // white/black noise pattern based on world position
  float check_size = 0.02f;
  float x = floorf(ctx->world_pos.x / check_size);
  float z = floorf(ctx->world_pos.y / check_size);

  float intensity = map_range(noise2D(x, z, g_world_config.seed), -1.0f, 1.0f, 0.55f, 1.0f);

  u8 r = (u8)(110.f * intensity);
  u8 g = (u8)(90.f * intensity);
  u8 b = (u8)(40.f * intensity);
  return default_lighting_frag_shader.func(rgb_to_u32(r, g, b), ctx, args, argc);
}

// Check if a point is in shadow from any tree
static bool point_in_tree_shadow(float3 world_pos, scene_t *scene) {
  if (!scene) return false;

  // Check current and neighboring chunks for tree shadows
  for (int dx = -1; dx <= 1; dx++) {
    for (int dz = -1; dz <= 1; dz++) {
      int chunk_x = (int)floorf(world_pos.x / g_world_config.chunk_size) + dx;
      int chunk_z = (int)floorf(world_pos.z / g_world_config.chunk_size) + dz;

      chunk_map_node_t *node = chunk_lookup((chunk_map_t*)&scene->chunk_map, chunk_x, chunk_z);
      if (!node || !node->loaded) continue;

      chunk_t *chunk = &node->chunk;

      // Check all trees in this chunk
      for (usize i = 0; i < chunk->num_trees; i++) {
        if (chunk->trees[i].num_vertices == 0) continue;

        // Recalculate tree position using same method as generation
        float world_x = chunk_x * g_world_config.chunk_size;
        float world_z = chunk_z * g_world_config.chunk_size;
        float tree_x = map_range(hash2(chunk_x * 100 + i, chunk_z * 100 + i * 3, g_world_config.seed), -1.0f, 1.0f, world_x + 2, world_x + g_world_config.chunk_size - 2);
        float tree_z = map_range(hash2(chunk_z * 100 + i * 7, chunk_x * 100 + i * 5, g_world_config.seed), -1.0f, 1.0f, world_z + 2, world_z + g_world_config.chunk_size - 2);

        // Simple circular shadow check
        float tree_dx = world_pos.x - tree_x;
        float tree_dz = world_pos.z - tree_z;
        float dist_sq = tree_dx * tree_dx + tree_dz * tree_dz;

        if (dist_sq < 3.24f) { // 1.8 * 1.8 = 3.24
          return true;
        }
      }
    }
  }

  return false;
}

// Shadow-enabled ground shader
u32 ground_shadow_func(u32 input, fragment_context_t *ctx, void *args, usize argc) {
  (void)input;

  // Get the actual terrain height at this world position
  float terrain_height = get_interpolated_terrain_height(ctx->world_pos.x, ctx->world_pos.z);

  u32 base_color;

  // Frozen lake ice texture
  if (terrain_height <= 0.01f) {
    // Ice surface variation
    float ice_variation = ridgeNoise(ctx->world_pos.x * 0.1f, ctx->world_pos.z * 0.1f, g_world_config.seed + 100);
    ice_variation = map_range(ice_variation, 0.0f, 1.0f, 0.4f, 1.6f);

    // Crack detection using gradient edge detection
    float crack_freq = 0.6f;
    float crack_sample_offset = 0.1f;

    // Primary crack layer
    float crack1 = ridgeNoise(ctx->world_pos.x * crack_freq, ctx->world_pos.z * crack_freq, g_world_config.seed + 200);
    float crack1_x = ridgeNoise((ctx->world_pos.x + crack_sample_offset) * crack_freq, ctx->world_pos.z * crack_freq, g_world_config.seed + 200);
    float crack1_z = ridgeNoise(ctx->world_pos.x * crack_freq, (ctx->world_pos.z + crack_sample_offset) * crack_freq, g_world_config.seed + 200);

    // Secondary crack layer
    float crack2_freq = crack_freq * 0.7f;
    float crack2 = ridgeNoise(ctx->world_pos.x * crack2_freq, ctx->world_pos.z * crack2_freq, g_world_config.seed + 300);
    float crack2_x = ridgeNoise((ctx->world_pos.x + crack_sample_offset) * crack2_freq, ctx->world_pos.z * crack2_freq, g_world_config.seed + 300);
    float crack2_z = ridgeNoise(ctx->world_pos.x * crack2_freq, (ctx->world_pos.z + crack_sample_offset) * crack2_freq, g_world_config.seed + 300);

    // Calculate crack edges from gradients
    float edge1 = fabsf(crack1_x - crack1) + fabsf(crack1_z - crack1);
    float edge2 = fabsf(crack2_x - crack2) + fabsf(crack2_z - crack2);
    float crack_strength = (fmaxf(edge1, edge2) > 0.15f) ? 3.0f : 1.0f;

    // Apply ice color with crack brightening
    float r = 45.0f * ice_variation * crack_strength;
    float g = 65.0f * ice_variation * crack_strength;
    float b = 120.0f * ice_variation * crack_strength;

    base_color = rgb_to_u32(
      (u8)(r > 255.0f ? 255 : (r < 0 ? 0 : r)),
      (u8)(g > 255.0f ? 255 : (g < 0 ? 0 : g)),
      (u8)(b > 255.0f ? 255 : (b < 0 ? 0 : b))
    );
  }
  // Shore gravel texture
  else if (terrain_height <= 0.3f) {
    // Pixelated gravel coordinates
    float gravel_x = floorf(ctx->world_pos.x / 0.15f);
    float gravel_z = floorf(ctx->world_pos.z / 0.15f);

    // Gravel base color and texture
    float gravel_base = noise2D(gravel_x, gravel_z, g_world_config.seed + 500);
    float gravel_ridge = ridgeNoise(gravel_x * 0.7f, gravel_z * 0.7f, g_world_config.seed + 600);
    float gravel_intensity = map_range(gravel_base, -1.0f, 1.0f, 0.5f, 1.3f) + gravel_ridge * 0.4f;

    // White stone chance (15%)
    float stone_chance = map_range(hash2((int)gravel_x, (int)gravel_z, g_world_config.seed + 700), -1.0f, 1.0f, 0.0f, 1.0f);

    if (stone_chance > 0.85f) {
      // White stones
      float white_brightness = map_range(stone_chance, 0.85f, 1.0f, 176.0f, 225.0f);
      base_color = rgb_to_u32((u8)white_brightness, (u8)white_brightness, (u8)(white_brightness + 5));
    } else {
      // Gray gravel
      float gray = 60.0f * gravel_intensity;
      base_color = rgb_to_u32(
        (u8)(gray > 255.0f ? 255 : (gray < 0 ? 0 : gray)),
        (u8)(gray > 255.0f ? 255 : (gray < 0 ? 0 : gray)),
        (u8)((gray + 10.0f) > 255.0f ? 255 : ((gray + 10.0f) < 0 ? 0 : (gray + 10.0f)))
      );
    }
  }
  else {
    // Generate normal ground color for higher terrain
    float check_size = 0.05f;
    float x = floorf(ctx->world_pos.x / check_size);
    float z = floorf(ctx->world_pos.z / check_size);

    float intensity = map_range(noise2D(x, z, g_world_config.seed), -1.0f, 1.0f, 0.85f, 1.0f);

    u8 r = (u8)(255.f * intensity);
    u8 g = (u8)(255.f * intensity);
    u8 b = (u8)(255.f * intensity);
    base_color = rgb_to_u32(r, g, b);
  }

  u32 lit_color = default_lighting_frag_shader.func(base_color, ctx, NULL, 0);

  // Apply tree shadows if scene data is available
  if (args && argc > 0) {
    scene_t *scene = (scene_t*)args;

    if (point_in_tree_shadow(ctx->world_pos, scene)) {
      // Darken the pixel by 50%
      u8 shadow_r, shadow_g, shadow_b;
      u32_to_rgb(lit_color, &shadow_r, &shadow_g, &shadow_b);
      shadow_r = (u8)(shadow_r * 0.5f);
      shadow_g = (u8)(shadow_g * 0.5f);
      shadow_b = (u8)(shadow_b * 0.5f);
      return rgb_to_u32(shadow_r, shadow_g, shadow_b);
    }
  }

  return lit_color;
}

// Global scene pointer for shadow calculations
static scene_t *g_scene_for_shadows = NULL;


// White fragment shader for quads
u32 white_frag_func(u32 input, fragment_context_t *ctx, void *args, usize argc) {
  (void)input; (void)ctx; (void)args; (void)argc;
  return rgb_to_u32(255, 255, 255);
}

fragment_shader_t ground_shadow_frag = { .func = ground_shadow_func, .argv = NULL, .argc = 0, .valid = true };
fragment_shader_t tree_frag = { .func = tree_frag_func, .argv = NULL, .argc = 0, .valid = true};
fragment_shader_t white_frag = { .func = white_frag_func, .argv = NULL, .argc = 0, .valid = true};

// Function to set the scene data for shadow calculations
void set_shadow_scene(scene_t *scene) {
  g_scene_for_shadows = scene;
  // Update the shader arguments to point to the scene
  ground_shadow_frag.argv = g_scene_for_shadows;
  ground_shadow_frag.argc = sizeof(scene_t);
}
