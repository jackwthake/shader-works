#ifndef __DOG_H__
#define __DOG_H__

#include <shader-works/maths.h>
#include <shader-works/renderer.h>

typedef enum {
  DOG_STATE_IDLE,
  DOG_STATE_WALKING,
  DOG_STATE_RUNNING,
  DOG_STATE_SITTING,
  DOG_STATE_LYING_DOWN
} dog_state_t;

typedef struct {
  float3 position;
  float3 velocity;
  float3 color;
  float size;
  dog_state_t state;

  model_t model;
  bool active;
} dog_t;

void init_dog(dog_t *dog, float3 position, float3 color, float size);
void update_dog(dog_t *dog, float delta_time);

#endif // __DOG_H__