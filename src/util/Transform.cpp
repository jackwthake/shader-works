#include "Transform.h"

#include <cmath>

float3 Transform::to_world_point(float3 p) {
  float3 ihat, jhat, khat;
  get_basis_vectors(ihat, jhat, khat);

  return transform_vector(ihat, jhat, khat, p) + position;
}

void Transform::get_basis_vectors(float3 &ihat, float3 &jhat, float3 &khat) {
  float3 ihat_yaw = float3(cos(this->yaw), 0, sin(this->yaw));
  float3 jhat_yaw = float3(0, 1, 0);
  float3 khat_yaw = float3(-sin(yaw), 0, cos(yaw));

  float3 ihat_pitch = float3(1, 0, 0);
  float3 jhat_pitch = float3(0, cos(pitch), -sin(pitch));
  float3 khat_pitch = float3(0, sin(pitch), cos(pitch));

  ihat = transform_vector(ihat_yaw, jhat_yaw, khat_yaw, ihat_pitch);
  jhat = transform_vector(ihat_yaw, jhat_yaw, khat_yaw, jhat_pitch);
  khat = transform_vector(ihat_yaw, jhat_yaw, khat_yaw, khat_pitch);
}

float3 Transform::transform_vector(float3 ihat, float3 jhat, float3 khat, float3 v) {
  return ihat * v.x + jhat * v.y + khat * v.z;
}