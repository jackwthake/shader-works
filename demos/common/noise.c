#include "noise.h"
#include <math.h>

// Smooth step function for better interpolation
inline float smoothstep(float t) {
  return t * t * (3.0f - 2.0f * t);
}

// Simple 1D interpolation
inline float lerp(float a, float b, float t) {
  return a + t * (b - a);
}

// Hash function to get pseudo-random gradients with seed
inline float hash2(int x, int y, int seed) {
  int n = x + y * 57 + seed * 2654435761;
  n = (n << 13) ^ n;
  return (1.0f - (float)((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

// Simple 2D Perlin-like noise (using a hash for gradients) with seed
float noise2D(float x, float y, int seed) {
  // Grid coordinates - use floor for proper handling of negative coordinates
  int xi = (int)floorf(x);
  int yi = (int)floorf(y);

  // Fractional parts
  float xf = x - (float)xi;
  float yf = y - (float)yi;

  // Hash function to get pseudo-random gradients at grid points
  // (This is simplified - you'd want better hashing in practice)
  float a = hash2(xi, yi, seed);
  float b = hash2(xi + 1, yi, seed);
  float c = hash2(xi, yi + 1, seed);
  float d = hash2(xi + 1, yi + 1, seed);

  // Interpolate
  float i1 = lerp(a, b, smoothstep(xf));
  float i2 = lerp(c, d, smoothstep(xf));
  return lerp(i1, i2, smoothstep(yf));
}

// Fractal Brownian Motion (fBm) for generating terrain with seed
float fbm(float x, float y, int octaves, int seed) {
  float value = 0.0f;
  float amplitude = 1.0f;
  float frequency = 1.0f;
  float maxValue = 0.0f;  // For normalizing

  for (int i = 0; i < octaves; i++) {
    value += noise2D(x * frequency, y * frequency, seed + i) * amplitude;
    maxValue += amplitude;

    amplitude *= 0.5f;  // Each octave has half the amplitude
    frequency *= 2.0f;  // Each octave has double the frequency
  }

  return value / maxValue;  // Normalize to [-1, 1]
}

// Ridge noise function to create sharp features with seed
float ridgeNoise(float x, float y, int seed) {
  float n = fbm(x, y, 4, seed);
  return 1.0f - fabsf(n);  // Creates ridges by inverting absolute value
}
