#include <stdio.h>
#include <float.h>
#include <math.h>

#include <cpu-render/renderer.h>
#include <cpu-render/maths.h>

#include "shaders.h"

// Fragment Shaders

// Simple blue shader
u32 frag_cube_func(u32 input, fragment_context_t context, void *argv, usize argc) {
  if (input == 0x00000000) return 0x00000000;
  return default_lighting_frag_shader.func(rgb_to_u32(50, 50, 150), context, argv, argc);
}

// Soft green shader with noise based on depth
u32 frag_plane_func(u32 input, fragment_context_t context, void *argv, usize argc) {
  if (input == 0x00000000)
    return 0x00000000;

  // Pixelated, fast-scrolling noise using quantized world position and time
  float pixel_size = 0.1f; // Controls pixelation
  float quant_x = floorf(context.world_pos.x / pixel_size) * pixel_size;
  float quant_y = floorf(context.world_pos.y / pixel_size) * pixel_size;
  float quant_z = floorf(context.world_pos.z / pixel_size) * pixel_size;

  float noise_x = sinf(quant_x * 8.0f + context.time * 15.0f) *
                  cosf(quant_z * 6.0f + context.time * 14.0f);
  float noise_y = sinf(quant_y * 10.0f + context.time * 16.0f) *
                  cosf(quant_x * 7.0f);

  float noise = (noise_x + noise_y) * 0.5f;

  u8 r = (u8)fmaxf(0.0f, fminf(170.0f, 50.0f + noise * 100.0f));
  u8 g = (u8)fmaxf(0.0f, fminf(255.0f, 200.0f + noise * 80.0f));
  u8 b = (u8)fmaxf(0.0f, fminf(150.0f, 30.0f + noise * 60.0f));

  u32 output = default_lighting_frag_shader.func(rgb_to_u32(r, g, b), context, argv, argc);
  return output;
}

// Soft blue shader with depth and time-based animation
u32 frag_sphere_func(u32 input, fragment_context_t context, void *argv, usize argc) {
  if (input == 0x00000000)
    return 0x00000000;

  // Demonstrate context usage - animate color based on depth and time
  float depth_factor = fmaxf(0.0f, fminf(1.0f, context.world_pos.y / 2.0f));
  float time_wave = (sinf(context.time * 2.0f) + 1.0f) * 0.5f;

  u8 r = (u8)(100 + depth_factor * 155 * time_wave);
  u8 g = (u8)(100 + (1.0f - depth_factor) * 155);
  u8 b = (u8)(255 - depth_factor * 100);

  u32 output = default_lighting_frag_shader.func(rgb_to_u32(r, g, b), context, argv, argc);
  return output;
}

// Particle fragment shader - circular particle
u32 particle_frag_func(u32 input, fragment_context_t context, void *argv, usize argc) {
  // Center the UV coordinates around (0.5, 0.5)
  f32 dx = context.uv.x - 0.5f;
  f32 dy = context.uv.y - 0.5f;
  f32 dist = sqrtf(dx * dx + dy * dy);

  // Use maximum radius to fill most of the billboard
  f32 radius = 0.5f;

  // Hard circle cutoff - discard pixels outside circle
  if (dist > radius)
    return rgb_to_u32(255, 0, 255); // Transparent (discard pixel)

  return input; // Use input texture color
}

// Vertex Shaders

// Vertex shader for plane ripple effect
float3 plane_ripple_vertex_shader(vertex_context_t context, void *argv, usize argc) {
  float3 vertex = context.original_vertex;

  // Apply the same ripple effect that was previously done in CPU
  vertex.y = (sinf(context.time + vertex.x) * 0.25f) /
             (sinf(context.time * 5.0f + (vertex.x * 2.0f)) / 4.0f + 1.0f);

  // Update normal to reflect the rippled surface
  float delta = 0.01f;
  float3 p_dx = (float3){ vertex.x + delta, 0, vertex.z };
  float3 p_dz = (float3){ vertex.x, 0, vertex.z + delta };
  p_dx.y = (sinf(context.time + p_dx.x) * 0.25f) /
           (sinf(context.time * 5.0f + (p_dx.x * 2.0f)) / 4.0f + 1.0f);
  p_dz.y = (sinf(context.time + p_dz.x) * 0.25f) /
           (sinf(context.time * 5.0f + (p_dz.x * 2.0f)) / 4.0f + 1.0f);
  float3 tangent = float3_sub(p_dx, vertex);
  float3 bitangent = float3_sub(p_dz, vertex);
  float3 normal = float3_cross(tangent, bitangent);
  normal = float3_normalize(normal);

  *context.original_normal = normal;

  return vertex;
}

// Vertex shader for sphere blob effect
float3 sphere_blob_vertex_shader(vertex_context_t context, void *argv, usize argc) {
  float3 vertex = context.original_vertex;

  // Calculate distance from origin for noise basis
  float base_distance = sqrtf(vertex.x * vertex.x + vertex.y * vertex.y + vertex.z * vertex.z);

  // Multiple noise frequencies for organic blob effect
  float noise1 = sinf(context.time * 2.0f + vertex.x * 4.0f + vertex.y * 3.0f + vertex.z * 2.0f);
  float noise2 = sinf(context.time * 1.5f + vertex.y * 5.0f + vertex.z * 4.0f);
  float noise3 = cosf(context.time * 3.0f + vertex.z * 3.0f + vertex.x * 2.0f);

  // Combine noise for organic deformation
  float combined_noise = (noise1 * 0.4f + noise2 * 0.3f + noise3 * 0.3f);

  // Add slow breathing motion
  float breathing = sinf(context.time * 0.8f) * 0.015f;

  // Calculate deformation factor - stronger at poles, weaker at equator
  float pole_factor = fabsf(vertex.y) / base_distance;
  float deform_strength = 0.3f + pole_factor * 0.2f;

  // Apply deformation along the normal direction (original vertex is normalized for sphere)
  float3 normal = vertex;  // For unit sphere, position is the normal
  if (base_distance > 0.001f) {
    normal.x /= base_distance;
    normal.y /= base_distance;
    normal.z /= base_distance;
  }

  // Apply the blob deformation with bounds checking
  float displacement = (combined_noise * deform_strength + breathing) * 0.25f;

  // Clamp displacement to prevent extreme deformations
  displacement = fmaxf(-0.4f, fminf(0.4f, displacement));

  vertex.x += normal.x * displacement;
  vertex.y += normal.y * displacement;
  vertex.z += normal.z * displacement;

  return vertex;
}

// Billboard vertex shader - makes quads always face the camera
float3 billboard_vertex_shader(vertex_context_t context, void *argv, usize argc) {
  float3 vertex = context.original_vertex;

  // Get camera vectors from context
  float3 cam_right = context.cam_right;
  float3 cam_up = context.cam_up;

  // Billboard position is stored in the model transform
  // The vertex coordinates represent offsets from the billboard center
  float3 world_pos = float3_add(
    float3_add(
      float3_scale(cam_right, vertex.x),  // X offset using camera right vector
      float3_scale(cam_up, vertex.y)     // Y offset using camera up vector
    ),
    make_float3(0.0f, 0.0f, 0.0f)        // Billboard center will be added by transform
  );

  return world_pos;
}