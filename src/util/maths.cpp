#include "maths.h"

#include <cmath>

/**
 * A simple 3D vector structure to represent 2D coordinates or vectors.
 */

float float3::dot(float3 a, float3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

float3 float3::normalize(const float3& vec) {
  float magnitude_sq = vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
  
  // Check if the magnitude is close to zero to prevent division by zero.
  if (magnitude_sq < 1e-6f) {
    // Normalizing a zero vector is undefined, so returning zero is a common practice.
    return float3(0.0f, 0.0f, 0.0f);
  }
  
  // Calculate the actual magnitude (length) of the vector.
  float magnitude = std::sqrt(magnitude_sq);
  
  // Return a new float3 object with each component divided by the magnitude.
  return float3(vec.x / magnitude, vec.y / magnitude, vec.z / magnitude);
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


float3 float3::operator+=(const float3& other) {
  // Add the x components
  x += other.x;
  // Add the y components
  y += other.y;
  // Add the z components
  z += other.z;
  // Return a reference to the modified current object
  return *this;
}


float3 float3::operator-=(const float3& other) {
  // Add the x components
  x += other.x;
  // Add the y components
  y += other.y;
  // Add the z components
  z += other.z;
  // Return a reference to the modified current object
  return *this;
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


float3 operator/(float scaler, const float3 &rhs) {
  if (scaler == 0)
    return rhs;

  return float3(scaler / rhs.x, scaler / rhs.y, scaler / rhs.z);
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
float3 random_color() {
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


/*
 * Implementation of transform class methods
*/

float3 Transform::to_world_point(float3 p) {
  float3 ihat, jhat, khat;
  get_basis_vectors(ihat, jhat, khat);

  return transform_vector(ihat, jhat, khat, p) + position;
}


float3 Transform::to_local_point(float3 world_point) {
  float3 ihat, jhat, khat;
  get_inverse_basis_vectors(ihat, jhat, khat);

  return transform_vector(ihat, jhat, khat, world_point - position);
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


void Transform::get_inverse_basis_vectors(float3 &ihat, float3 &jhat, float3 &khat) {
    float3 ihat_yaw = float3(cos(-this->yaw), 0, sin(-this->yaw));
    float3 jhat_yaw = float3(0, 1, 0);
    float3 khat_yaw = float3(-sin(-yaw), 0, cos(-yaw));

    float3 ihat_pitch = float3(1, 0, 0);
    float3 jhat_pitch = float3(0, cos(-pitch), -sin(-pitch));
    float3 khat_pitch = float3(0, sin(-pitch), cos(-pitch));

    ihat = transform_vector(ihat_yaw, jhat_yaw, khat_yaw, ihat_pitch);
    jhat = transform_vector(ihat_yaw, jhat_yaw, khat_yaw, jhat_pitch);
    khat = transform_vector(ihat_yaw, jhat_yaw, khat_yaw, khat_pitch);
}


float3 Transform::transform_vector(float3 ihat, float3 jhat, float3 khat, float3 v) {
  return ihat * v.x + jhat * v.y + khat * v.z;
}