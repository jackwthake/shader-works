#ifndef PLATFORM_H
#define PLATFORM_H

extern "C" {
  #include <shader-works/primitives.h>
};

#include "scene.hpp"

void update_timing(fps_controller_t &controller);
void handle_input(fps_controller_t &controller, transform_t &camera);

#endif
