#ifndef NOISE_H
#define NOISE_H

float smoothstep(float t);
float lerp(float a, float b, float t);
float hash2(int x, int y, int seed);

float noise2D(float x, float y, int seed);
float fbm(float x, float y, int octaves, int seed);
float ridgeNoise(float x, float y, int seed);

#endif
