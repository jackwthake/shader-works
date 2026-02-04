#ifndef __STAGES_H__
#define __STAGES_H__

#include "../util/state.h"
#include "../const.h"

// Scene state interfaces
extern state_interface_t scene_menu;

// part 1
extern state_interface_t scene_01;
extern state_interface_t scene_02;

// part 2
extern state_interface_t scene_03;
extern state_interface_t scene_04;
extern state_interface_t scene_05;

// part 3
extern state_interface_t scene_06;
extern state_interface_t scene_07;
extern state_interface_t scene_08;

typedef enum {
    SCENE_MENU = 0,
    SCENE_01,
    SCENE_02,
    SCENE_03,
    SCENE_04,
    SCENE_05,
    SCENE_06,
    SCENE_07,
    SCENE_08,
} scene_index_t;

void init_player_camera(context_t *ctx);

void apply_fps_movement(context_t *ctx, float dt);

int render_scene(context_t *ctx, float fog_start);

void get_cycle_color(float time_elapsed, const u8 colors[][3], u8 *r, u8 *g, u8 *b);
float get_distance_from_origin(float3 pos);

#endif // __STAGES_H__