#include "vec.h"

#include <stdlib.h>

float float3::dot(float3 a, float3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

float3 float3::operator+(const float3& other) const {
  return float3(x + other.x, y + other.y, z + other.z);
}

float3 float3::operator-(const float3& other) const {
  return float3(x - other.x, y - other.y, z - other.z);
}

float3 float3::operator*(float scalar) const {
  return float3(x * scalar, y * scalar, z * scalar);
}

float3 float3::operator/(float scalar) const {
  if (scalar != 0) {
    return float3(x / scalar, y / scalar, z / scalar);
  }
  return *this; // Avoid division by zero
}


/**
 * A simple 2D vector structure to represent 2D coordinates or vectors.
 */

float2 float2::get_purpendicular(float2 vec) {
  return float2(vec.y, -vec.x);
}

float float2::dot(float2 a, float2 b) {
  return a.x * b.x + a.y * b.y;
}

float2 float2::operator+(const float2& other) const {
  return float2(x + other.x, y + other.y);
}

float2 float2::operator-(const float2& other) const {
  return float2(x - other.x, y - other.y);
}

float2 float2::operator*(float scalar) const {
  return float2(x * scalar, y * scalar);
}

float2 float2::operator/(float scalar) const {
  if (scalar != 0) {
    return float2(x / scalar, y / scalar);
  }
  return *this; // Avoid division by zero
}



/**
 * Generates a random float3 with each component in the range [0, 255].
 * @return A random float3.
 */
float3 random_colour() {
  float x = static_cast<float>(rand()) / RAND_MAX * 255.0f;
  float y = static_cast<float>(rand()) / RAND_MAX * 255.0f;
  float z = static_cast<float>(rand()) / RAND_MAX * 255.0f;
  return float3(x, y, z);
}


/**
 * Generates a random float2 within the specified max bounds, min is always 0.
 * @param maxX Maximum x value.
 * @param maxY Maximum y value.
 * @return A random float2.
 */
float2 random_float2(float maxX, float maxY) {
  float x = static_cast<float>(rand()) / RAND_MAX * maxX;
  float y = static_cast<float>(rand()) / RAND_MAX * maxY;
  return float2(x, y);
}