#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>
#include <string>

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


struct Model {
  vector<float3> vertices; // List of vertices
  vector<float3> cols; // List of vertex colors
};


struct RenderState {
  int width, height; // Dimensions of the render target
  float3* pixels; // Pointer to pixel data

  float2 dim() const {
    return float2(static_cast<float>(width), static_cast<float>(height));
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
 * Reads vertices and face indices from an OBJ file.
 * Only supports 'v' (vertex) and 'f' (face) lines.
 * Returns a vector of float3 containing all vertices used in faces (in face order).
 */
vector<float3> read_obj_vertices(const string& filename) {
  ifstream file(filename);

  vector<float3> vertices;
  vector<float3> out_vertices;
  string line;

  if (!file) {
    cerr << "Failed to open OBJ file: " << filename << endl;
    return out_vertices;
  }

  // Read the file line by line
  while (getline(file, line)) {
    if (line.empty()) continue;

    istringstream iss(line);
    string prefix;
    iss >> prefix;
    
    if (prefix == "v") {
      float x, y, z;
      iss >> x >> y >> z;

      vertices.push_back(float3(x, y, z));
    } else if (prefix == "f") {
      int idx[4], count = 0;
      string vert;

      // OBJ indices are 1-based
      while (iss >> vert && count < 4) {
        size_t slash = vert.find('/');
        int v_idx = stoi(slash == string::npos ? vert : vert.substr(0, slash));

        if (v_idx < 0) v_idx = vertices.size() + v_idx + 1;
        out_vertices.push_back(vertices[v_idx - 1]);
        count++;
      }
    }
  }

  file.close();
  return out_vertices;
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
 * Converts 
 */
float2 vertex_to_screen(const float3 &vertex, float2 dim) {
  float screen_height_world = 5;
  float pixels_per_world_unit = dim.y / screen_height_world;

  float2 screen_pos;
  screen_pos.x = vertex.x * pixels_per_world_unit;
  screen_pos.y = vertex.y * pixels_per_world_unit;

  return (dim / 2) + screen_pos;
}


void render(Model &model, RenderState &state) {
  // Clear the pixel buffer
  for (int i = 0; i < state.width * state.height; ++i) {
    state.pixels[i] = float3(0, 0, 0); // Set to black
  }

  // Render each triangle in the model
  for (size_t i = 0; i < model.vertices.size(); i += 3) {
    float2 a = vertex_to_screen(model.vertices[i], state.dim());
    float2 b = vertex_to_screen(model.vertices[i + 1], state.dim());
    float2 c = vertex_to_screen(model.vertices[i + 2], state.dim());

    // Draw the triangle
    for (int y = 0; y < state.height; ++y) {
      for (int x = 0; x < state.width; ++x) {
        if (point_in_triangle(a, b, c, float2(x, y))) {
          state.pixels[x + state.width * y] = model.cols[i / 3]; // Use color from the first vertex of the triangle
        }
      }
    }
  }
}


/**
 * Creates a test image with a gradient pattern.
 * @param pixels Pointer to an array of float3 representing pixel colors.
 * @param width The width of the image.
 * @param height The height of the image.
 */
void create_test_image(RenderState &state) {
  vector<float3> cube = read_obj_vertices("cube.obj");
  if (cube.empty()) {
    cerr << "Failed to read vertices from cube.obj" << endl;
    return;
  }

  int num_triangles = cube.size() / 3;
  float3 triangle_cols[num_triangles];

  srand(time(NULL));

  // Generate the triangles and their colors
  for (int i = 0; i < num_triangles; ++i) {
    triangle_cols[i] = random_colour();
  }

  Model model;
  model.vertices = cube;

  model.cols.resize(num_triangles);
  for (int i = 0; i < num_triangles; ++i) {
    model.cols[i] = triangle_cols[i];
  }
  
  render(model, state);
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
  RenderState state;
  state.width = 160;
  state.height = 128;
  state.pixels = new float3[state.width * state.height];
  if (!state.pixels) {
    cerr << "Memory allocation failed." << endl;
    return EXIT_FAILURE;
  }

  create_test_image(state);
  write_bytes_to_bmp("test_image.bmp", state.pixels, state.width, state.height);
  cout << "BMP image created successfully: test_image.bmp" << endl;

  delete[] state.pixels;
  return EXIT_SUCCESS;
}