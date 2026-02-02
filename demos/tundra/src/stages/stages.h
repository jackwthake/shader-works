#ifndef __STAGES_H__
#define __STAGES_H__

#include "../util/state.h"

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

#endif // __STAGES_H__