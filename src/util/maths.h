#ifndef MATHS_H
#define MATHS_H


/**
 * A simple 3D vector structure to represent 2D coordinates or vectors.
 */
struct float3 {
  float x, y, z;

  float3() : x(0), y(0), z(0) {}
  float3(float x, float y, float z) : x(x), y(y), z(z) {}

  static float dot(float3 a, float3 b);

  float3 operator+(const float3& other) const;
  float3 operator-(const float3& other) const;
  float3 operator*(float scalar) const;
  float3 operator/(float scalar) const;
};


/**
 * A simple 2D vector structure to represent 2D coordinates or vectors.
 */
struct float2 {
  float x, y;

  float2() : x(0), y(0) {}
  float2(float x, float y) : x(x), y(y) {}
  float2(const class float3& v) : x(v.x), y(v.y) {}

  static float2 get_purpendicular(float2 vec);
  static float dot(float2 a, float2 b);

  float2 operator+(const float2& other) const;
  float2 operator-(const float2& other) const;
  float2 operator*(float scalar) const;
  float2 operator/(float scalar) const;
};

class Transform {
  public:
    float yaw;
    float pitch;
    float3 position;

  float3 to_world_point(float3 p);
  void get_basis_vectors(float3 &ihat, float3 &jhat, float3 &khat);
  float3 transform_vector(float3 ihat, float3 jhat, float3 khat, float3 v);
};

float2 random_float2(float maxX, float maxY);
float3 random_color();

#endif // MATHS_H