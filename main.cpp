#include <iostream>
#include <fstream>
#include <time.h>
#include <stdio.h>

using namespace std;

/**
 * A simple 3D vector structure to represent RGB colors or 3D Coordinates.
 */
struct float3 {
  float x, y, z;

  float3() : x(0), y(0), z(0) {}
  float3(float x, float y, float z) : x(x), y(y), z(z) {}

  float3 operator+(const float3& other) const {
    return float3(x + other.x, y + other.y, z + other.z);
  }

  float3 operator-(const float3& other) const {
    return float3(x - other.x, y - other.y, z - other.z);
  }

  float3 operator*(float scalar) const {
    return float3(x * scalar, y * scalar, z * scalar);
  }

  float3 operator/(float scalar) const {
    if (scalar != 0) {
      return float3(x / scalar, y / scalar, z / scalar);
    }
    return *this; // Avoid division by zero
  }
};


/**
 * A simple 2D vector structure to represent 2D coordinates or vectors.
 */
struct float2 {
  float x, y;

  float2() : x(0), y(0) {}
  float2(float x, float y) : x(x), y(y) {}

  float2 operator+(const float2& other) const {
    return float2(x + other.x, y + other.y);
  }

  float2 operator-(const float2& other) const {
    return float2(x - other.x, y - other.y);
  }

  float2 operator*(float scalar) const {
    return float2(x * scalar, y * scalar);
  }

  float2 operator/(float scalar) const {
    if (scalar != 0) {
      return float2(x / scalar, y / scalar);
    }
    return *this; // Avoid division by zero
  }
};


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


/**
 * Helper function to check if a point is inside a triangle using barycentric coordinates.
 */
bool point_in_triangle(const float2& a, const float2& b, const float2& c, const float2& p) {
  float2 v0 = c - a;
  float2 v1 = b - a;
  float2 v2 = p - a;

  float d00 = v0.x * v0.x + v0.y * v0.y;
  float d01 = v0.x * v1.x + v0.y * v1.y;
  float d11 = v1.x * v1.x + v1.y * v1.y;
  float d20 = v2.x * v0.x + v2.y * v0.y;
  float d21 = v2.x * v1.x + v2.y * v1.y;

  float denom = d00 * d11 - d01 * d01;
  if (denom == 0.0f) return false;
  float v = (d11 * d20 - d01 * d21) / denom;
  float w = (d00 * d21 - d01 * d20) / denom;
  float u = 1.0f - v - w;
  return (u >= 0) && (v >= 0) && (w >= 0);
}


/**
 * Creates a test image with a gradient pattern.
 * @param pixels Pointer to an array of float3 representing pixel colors.
 * @param width The width of the image.
 * @param height The height of the image.
 */
void create_test_image(float3* pixels, int width, int height) {
  const int triangle_count = 30, num_points = triangle_count * 3;
  float2* points = new float2[num_points];
  float3* triangle_cols = new float3[triangle_count];

  float2 half_size(width / 2, height / 2);

  srand(time(NULL));

  // Generate the triangles and their colors
  for (int i = 0; i < triangle_count; ++i) {
    points[i] = random_float2(width, height);
    triangle_cols[i / 3] = random_colour();
  }

  // render each traingle to the texture
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      for (int i = 0; i < num_points; ++i) {
        float2 a = points[i];
        float2 b = points[i + 1];
        float2 c = points[i + 2];

        if (point_in_triangle(a, b, c, float2(x, y)))
          pixels[x + width * y] = triangle_cols[i / 3];
      }
    }
  }
}


/**
 * Writes pixel data to a BMP file.
 * @param filename The name of the file to write.
 * @param pixels Pointer to an array of float3 representing pixel colors.
 * @param width The width of the image.
 * @param height The height of the image.
 */
void write_bytes_to_bmp(string filename, const float3* pixels, int width, int height) {
  ofstream file(filename, ios::binary);
  if (!file) {
    cerr << "Error opening file for writing: " << filename << endl;
    return;
  }

  // BMP file header
  uint32_t fileSize = 54 + width * height * 3; // 54 bytes for header + pixel data
  uint32_t offset = 54; // Offset to pixel data
  uint16_t reserved1 = 0;
  uint16_t reserved2 = 0;
  uint32_t headerSize = 40; // Size of the DIB header
  uint16_t planes = 1; // Number of color planes
  uint16_t bitsPerPixel = 24; // Bits per pixel
  uint32_t compression = 0; // No compression
  uint32_t imageSize = width * height * 3; // Size of pixel data
  uint32_t xPixelsPerMeter = 2835; // 72 DPI
  uint32_t yPixelsPerMeter = 2835; // 72 DPI
  uint32_t colorsUsed = 0; // No palette
  uint32_t importantColors = 0; // All colors are important

  // Write BMP file header
  file.put('B').put('M'); // Signature
  file.write(reinterpret_cast<const char*>(&fileSize), sizeof(fileSize));
  file.write(reinterpret_cast<const char*>(&reserved1), sizeof(reserved1));
  file.write(reinterpret_cast<const char*>(&reserved2), sizeof(reserved2));
  file.write(reinterpret_cast<const char*>(&offset), sizeof(offset));
  file.write(reinterpret_cast<const char*>(&headerSize), sizeof(headerSize));
  file.write(reinterpret_cast<const char*>(&width), sizeof(width));
  file.write(reinterpret_cast<const char*>(&height), sizeof(height));
  file.write(reinterpret_cast<const char*>(&planes), sizeof(planes));
  file.write(reinterpret_cast<const char*>(&bitsPerPixel), sizeof(bitsPerPixel));
  file.write(reinterpret_cast<const char*>(&compression), sizeof(compression));
  file.write(reinterpret_cast<const char*>(&imageSize), sizeof(imageSize));
  file.write(reinterpret_cast<const char*>(&xPixelsPerMeter), sizeof(xPixelsPerMeter));
  file.write(reinterpret_cast<const char*>(&yPixelsPerMeter), sizeof(yPixelsPerMeter));
  file.write(reinterpret_cast<const char*>(&colorsUsed), sizeof(colorsUsed));
  file.write(reinterpret_cast<const char*>(&importantColors), sizeof(importantColors));
  
  // Write pixel data
  for (int y = height - 1; y >= 0; --y) { // BMP format stores pixels bottom to top
    for (int x = 0; x < width; ++x) {
      const float3& pixel = pixels[y * width + x];
      uint8_t r = static_cast<uint8_t>(pixel.x * 255);
      uint8_t g = static_cast<uint8_t>(pixel.y * 255);
      uint8_t b = static_cast<uint8_t>(pixel.z * 255);
      file.put(b).put(g).put(r); // BMP uses BGR format
    }
  }

  file.close();
}


int main(void) {
  int width = 160, height = 128;
  float3* pixels = new float3[width * height];
  if (!pixels) {
    cerr << "Memory allocation failed." << endl;
    return EXIT_FAILURE;
  }

  create_test_image(pixels, width, height);
  write_bytes_to_bmp("test_image.bmp", pixels, width, height);
  cout << "BMP image created successfully: test_image.bmp" << endl;

  delete[] pixels;
  return EXIT_SUCCESS;
}