#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__

#include "vec.h"

class Transform {
  public:
    float yaw;
    float pitch;
    float3 position;

  float3 to_world_point(float3 p);
  void get_basis_vectors(float3 &ihat, float3 &jhat, float3 &khat);
  float3 transform_vector(float3 ihat, float3 jhat, float3 khat, float3 v);
};

#endif
