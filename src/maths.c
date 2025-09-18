#include "maths.h"

#include <math.h>

/** functions to make instances of a vector */
float3 make_float3(f32 x, f32 y, f32 z) {
  return (float3){x, y, z};
}

float2 make_float2(f32 x, f32 y) {
  return (float2){x, y};
}

/** float3 implementation */
float3 float3_add(float3 a, float3 b) {
  return (float3){a.x + b.x, a.y + b.y, a.z + b.z};
}

float3 float3_sub(float3 a, float3 b) {
  return (float3){a.x - b.x, a.y - b.y, a.z - b.z}; 
}

float3 float3_scale(float3 v, float s) {
  return (float3){v.x * s, v.y * s, v.z * s};
}

float3 float3_divide(float3 v, float s) {
  return (float3){v.x / s, v.y / s, v.z / s};
}

f32 float3_dot(float3 a, float3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

f32 float3_magnitude(float3 v) {
  return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

float3 float3_normalize(float3 v) {
  f32 mag = float3_magnitude(v);
  if (mag == 0) return (float3){0, 0, 0};
  return float3_scale(v, 1.0f / mag);
}

float3 float3_cross(float3 a, float3 b) {
  return (float3){
    a.y * b.z - a.z * b.y,
    a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x
  };
}

/** float2 implementation */
float2 float2_add(float2 a, float2 b) {
  return (float2){a.x + b.x, a.y + b.y};
}

float2 float2_sub(float2 a, float2 b) {
  return (float2){a.x - b.x, a.y - b.y}; 
}

float2 float2_scale(float2 v, float s) {
  return (float2){v.x * s, v.y * s};
}

float2 float2_divide(float2 v, float s) {
  return (float2){v.x / s, v.y / s};
}

f32 float2_dot(float2 a, float2 b) {
  return a.x * b.x + a.y * b.y;
}

f32 float2_magnitude(float2 v) {
  return sqrtf(v.x * v.x + v.y * v.y);
}

float2 float2_normalize(float2 v) {
  f32 mag = float2_magnitude(v);
  if (mag == 0) return (float2){0, 0};
  return float2_scale(v, 1.0f / mag);
}