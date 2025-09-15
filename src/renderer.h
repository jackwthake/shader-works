#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>
#include "def.h"

#define MAX_DEPTH 12.0f
#define FOG_START 2.0f
#define FOG_END   11.5f  // End of fog effect
#define FOG_COLOR 0x344866

#define EPSILON 0.0001f
#define PI 3.14159265f
#define CUBE_VERTEX_COUNT 36

// --- Constants for the Atlas ---
#define ATLAS_WIDTH_PX 80
#define ATLAS_HEIGHT_PX 24
#define TILE_WIDTH_PX 8
#define TILE_HEIGHT_PX 8

// Derived constants
#define TILES_PER_ROW (ATLAS_WIDTH_PX / TILE_WIDTH_PX)
#define TILES_PER_COL (ATLAS_HEIGHT_PX / TILE_HEIGHT_PX)
#define TOTAL_TILES (TILES_PER_ROW * TILES_PER_COL)
#define TILE_PIXEL_COUNT (TILE_WIDTH_PX * TILE_HEIGHT_PX)

#define fov_over_2 (1.0472f / 2.0f) // 60 degrees in radians / 2

extern float3 cube_vertices[CUBE_VERTEX_COUNT];

typedef u32 (*shader_func)(u32 input_color, void *args, usize argc);
extern u32 default_shader(u32 input_color, void *args, usize argc);

void init_renderer(game_state_t *state);
void update_camera(game_state_t *state, transform_t *cam);
void render_model(game_state_t *state, transform_t *cam, model_t *model, shader_func frag_shader, void *shader_args, usize shader_argc);

#endif // RENDERER_H
