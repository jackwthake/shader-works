#ifndef FULL_TEST_SHADERS_H
#define FULL_TEST_SHADERS_H

#include <shader-works/renderer.h>

// Fragment Shader Functions
u32 frag_cube_func(u32 input, fragment_context_t context, void *argv, usize argc);
u32 frag_plane_func(u32 input, fragment_context_t context, void *argv, usize argc);
u32 frag_sphere_func(u32 input, fragment_context_t context, void *argv, usize argc);
u32 particle_frag_func(u32 input, fragment_context_t context, void *argv, usize argc);

// Vertex Shader Functions
float3 plane_ripple_vertex_shader(vertex_context_t context, void *argv, usize argc);
float3 sphere_blob_vertex_shader(vertex_context_t context, void *argv, usize argc);
float3 billboard_vertex_shader(vertex_context_t context, void *argv, usize argc);

#endif // FULL_TEST_SHADERS_H