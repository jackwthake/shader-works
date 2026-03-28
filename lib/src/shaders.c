#include <shader-works/shaders.h>
#include <shader-works/renderer.h>

#include <math.h>

// Default vertex shader that just returns the original vertex position
static inline float3 default_vertex_shader_func(vertex_context_t *context, void *args, usize argc) {
  return context->original_vertex;
}

// Default fragment shader that just returns the input color
static inline u32 default_frag_shader_func(u32 input_color, fragment_context_t *context, void *args, usize argc) {
  return input_color;
}

// Default lighting fragment shader that applies simple directional lighting
static inline u32 default_lighting_frag_shader_func(u32 input_color, fragment_context_t *context, void *args, usize argc) {
  if (input_color == 0x00000000)
    return 0x00000000;

  // Extract surface color components
  u8 surface_r, surface_g, surface_b;
  u32_to_rgb(input_color, &surface_r, &surface_g, &surface_b);

  float final_r, final_g, final_b;

  if (context->light_count == 0) {
    // No lights, use full surface color
    final_r = surface_r;
    final_g = surface_g;
    final_b = surface_b;
  } else {
    // Start with ambient light (small amount of original color)
    final_r = surface_r * 0.45;
    final_g = surface_g * 0.45f;
    final_b = surface_b * 0.45f;

    for (int i = 0; i < context->light_count; i++) {
      float light_contribution;
      if (context->light[i].is_directional) {
        // Directional light
        light_contribution = float3_dot(float3_normalize(context->light[i].direction), float3_normalize(context->normal));
      } else {
        // Point light - calculate direction from light to fragment
        float3 light_dir = float3_sub(context->world_pos, context->light[i].position);
        float distance = float3_magnitude(light_dir);
        light_dir = float3_divide(light_dir, distance); // normalize manually to reuse distance

        // Calculate lighting contribution
        light_contribution = float3_dot(light_dir, float3_normalize(context->normal));

        // Add distance falloff (gentler falloff)
        light_contribution = light_contribution / (1.0f + distance * 0.1f);
      }

      light_contribution = fmaxf(0.0f, light_contribution); // No negative lighting

      // Extract light color components
      u8 light_r, light_g, light_b;
      u32_to_rgb(context->light[i].color, &light_r, &light_g, &light_b);

      // Add colored light contribution
      final_r += surface_r * (light_r / 255.0f) * light_contribution;
      final_g += surface_g * (light_g / 255.0f) * light_contribution;
      final_b += surface_b * (light_b / 255.0f) * light_contribution;
    }

    // Clamp final colors
    final_r = fmaxf(0.0f, fminf(255.0f, final_r));
    final_g = fmaxf(0.0f, fminf(255.0f, final_g));
    final_b = fmaxf(0.0f, fminf(255.0f, final_b));
  }

  u8 r = (u8)final_r;
  u8 g = (u8)final_g;
  u8 b = (u8)final_b;

  return rgb_to_u32(r, g, b);
}

// Skybox fragment shader that samples from panoramic texture buffer
// argv should point to skybox_shader_args_t structure
u32 skybox_frag_shader_func(u32 input_color, fragment_context_t *context, void *args, usize argc) {
  skybox_shader_args_t *sargs = (skybox_shader_args_t *)args;

  // If args or buffer missing, return fallback
  if (!sargs || !sargs->skybox_buffer) {
    return rgb_to_u32(135, 206, 235);  // Sky blue fallback
  }

  // Map UV coordinates to buffer pixel coordinates
  // U maps to X (wraps horizontally for panoramic)
  // V maps to Y (clamps vertically)
  int tex_x = (int)(context->uv.x * (sargs->width - 1));
  int tex_y = (int)(context->uv.y * (sargs->height - 1));

  // Clamp to valid range
  if (tex_x < 0) tex_x = 0;
  if (tex_x >= (int)sargs->width) tex_x = sargs->width - 1;
  if (tex_y < 0) tex_y = 0;
  if (tex_y >= (int)sargs->height) tex_y = sargs->height - 1;

  // Sample from skybox buffer
  u32 color = sargs->skybox_buffer[tex_y * sargs->width + tex_x];
  return color;
}

// Built-in default vertex shader that just returns the original vertex position
vertex_shader_t default_vertex_shader = { .func = default_vertex_shader_func, .argv = NULL, .argc = 0, .valid = true };

// Built-in default shader that just returns the input color
fragment_shader_t default_frag_shader = { .func = default_frag_shader_func, .argv = NULL, .argc = 0, .valid = true };

// Built-in default lighting shader that applies simple lighting
fragment_shader_t default_lighting_frag_shader = { .func = default_lighting_frag_shader_func, .argv = NULL, .argc = 0, .valid = true };

// Built-in skybox shader that samples from panoramic texture
// Note: This is a template; actual skybox shader must be created with args via make_fragment_shader()
static fragment_shader_t skybox_shader_template = { .func = skybox_frag_shader_func, .argv = NULL, .argc = 0, .valid = true };

// Helper functions to create shaders
fragment_shader_t make_fragment_shader(fragment_shader_func func, void *argv, usize argc) {
  return (fragment_shader_t) {
    .func = func,
    .argv = argv,
    .argc = argc,
    .valid = true
  };
}

vertex_shader_t make_vertex_shader(vertex_shader_func func, void *argv, usize argc) {
  return (vertex_shader_t) {
    .func = func,
    .argv = argv,
    .argc = argc,
    .valid = true
  };
}
