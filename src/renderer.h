#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>
#include "def.h"

#define MAX_DEPTH 12.0f
#define FOG_START 2.0f
#define FOG_END   11.5f  // End of fog effect
#define FOG_R 22
#define FOG_G 35
#define FOG_B 65

#define EPSILON 0.0001f
#define PI 3.14159265f

// --- Constants for the Atlas ---
#define ATLAS_WIDTH_PX 8
#define ATLAS_HEIGHT_PX 8

#define fov_over_2 (1.0472f / 2.0f) // 60 degrees in radians / 2

typedef u32 (*shader_func)(u32 input_color, void *args, usize argc);
extern u32 default_shader(u32 input_color, void *args, usize argc);

u32 rgb_to_888(u8 r, u8 g, u8 b);

void init_renderer(game_state_t *state);
void apply_fog_to_screen(game_state_t *state);
void update_camera(game_state_t *state, transform_t *cam);
void render_model(game_state_t *state, transform_t *cam, model_t *model, shader_func frag_shader, void *shader_args, usize shader_argc);

#endif // RENDERER_H
