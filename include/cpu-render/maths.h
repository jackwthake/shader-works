#ifndef CPU_RENDER_MATHS_H
#define CPU_RENDER_MATHS_H

#include <stdint.h>
#include <stddef.h>

#define PI 3.14159265f

// Type definitions
typedef int8_t   i8;
typedef uint8_t  u8;
typedef int16_t  i16;
typedef uint16_t u16;
typedef int32_t  i32;
typedef uint32_t u32;
typedef int64_t  i64;
typedef uint64_t u64;
typedef size_t   usize;

typedef float    f32;
typedef double   f64;

typedef struct {
  f32 x, y;
} float2;

typedef struct {
  f32 x, y, z;
} float3;

#define EPSILON 0.0001f

// Float3 operations
float3 make_float3(f32 x, f32 y, f32 z);
float3 float3_add(float3 a, float3 b);
float3 float3_sub(float3 a, float3 b);
float3 float3_scale(float3 v, float s);
float3 float3_divide(float3 v, float s);
f32 float3_dot(float3 a, float3 b);
float3 float3_cross(float3 a, float3 b);
f32 float3_magnitude(float3 v);
float3 float3_normalize(float3 v);

// Float2 operations
float2 make_float2(f32 x, f32 y);
float2 float2_add(float2 a, float2 b);
float2 float2_sub(float2 a, float2 b);
float2 float2_scale(float2 v, float s);
float2 float2_divide(float2 v, float s);
f32 float2_dot(float2 a, float2 b);
f32 float2_magnitude(float2 v);
float2 float2_normalize(float2 v);

#endif // CPU_RENDER_MATHS_H