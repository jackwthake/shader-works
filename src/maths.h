#ifndef __MATHS_H__
#define __MATHS_H__

#include "def.h"

float3 make_float3(f32 x, f32 y, f32 z);
float2 make_float2(f32 x, f32 y);

float3 float3_add(float3 a, float3 b);
float3 float3_sub(float3 a, float3 b);
float3 float3_scale(float3 v, float s);
float3 float3_divide(float3 v, float s);
f32 float3_dot(float3 a, float3 b);
f32 float3_magnitude(float3 v);
float3 float3_normalize(float3 v);

float2 float2_add(float2 a, float2 b);
float2 float2_sub(float2 a, float2 b);
float2 float2_scale(float2 v, float s);
float2 float2_divide(float2 v, float s);
f32 float2_dot(float2 a, float2 b);
f32 float2_magnitude(float2 v);
float2 float2_normalize(float2 v);

#endif