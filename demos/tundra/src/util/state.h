#ifndef __STATE_H__
#define __STATE_H__

#include <stddef.h>

// Callbacks for state lifecycle and execution
typedef struct {
  void (*enter)(void *, size_t);              // Called when entering state
  void (*tick)(void *, size_t, float dt);     // Called each fixed timestep
  int (*render)(void *, size_t);              // Called each frame, returns triangle count
  void (*exit)(void *, size_t);               // Called when exiting state
} state_interface_t;

// Finite state machine managing game states
typedef struct {
  state_interface_t *states;
  unsigned num_states;

  void *game_state;                           // Shared state passed to callbacks
  size_t game_state_size;

  int default_state, current_state;
} state_machine_t;

// Initialize state machine with default state and total number of states
void fsm_init(state_machine_t *sm, unsigned default_state, unsigned num_states);

// Start the state machine by entering the current state
int fsm_start(state_machine_t *sm);

// Free state machine resources
void fsm_free(state_machine_t *sm);

// Register state interface callbacks for a specific state
int fsm_set_state_interface(state_machine_t *sm, unsigned state, state_interface_t *interface);

// Transition to a new state (calls exit on current, enter on new)
int fsm_change_state(state_machine_t *sm, unsigned new_state);

// Update the shared game state pointer passed to all callbacks
int fsm_update_internal_state(state_machine_t *sm, void *state, size_t size);

// Get the current state index
int fsm_get_state(state_machine_t *sm);

// Execute tick callback for current state with delta time
void fsm_tick_state(state_machine_t *sm, float dt);

// Execute render callback for current state, returns triangle count
int fsm_render_state(state_machine_t *sm);

#endif