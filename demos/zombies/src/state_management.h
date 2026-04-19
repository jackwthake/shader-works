#ifndef __GAME_STATE_H__
#define __GAME_STATE_H__

#include <stdbool.h>

#include "common/state.h"

enum {
  STATE_NORMAL,
  STATE_GENERATE_MAP,
  STATE_MAP_REGENERATION,
  STATE_NUM_STATES
};

extern state_machine_t game_state;
extern state_interface_t normal_state, generate_map_state, regenerate_map_state;

#endif // __GAME_STATE_H__
