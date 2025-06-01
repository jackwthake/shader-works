#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>
#include <string>
#include <math.h>

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

  static float2 get_purpendicular(float2 vec) {
    return float2(vec.y, -vec.x);
  }

  static float dot(float2 a, float2 b) {
    return a.x * b.x + a.y * b.y;
  }

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
 * A class to represent a transformation in 3D space.
 */
class Transform {
  public:
    float yaw;
    float pitch;
    float3 position;

    float3 to_world_point(float3 p) {
      float3 ihat, jhat, khat;
      get_basis_vectors(ihat, jhat, khat);

      return transform_vector(ihat, jhat, khat, p) + position;
    }

    void get_basis_vectors(float3 &ihat, float3 &jhat, float3 &khat) {
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

    float3 transform_vector(float3 ihat, float3 jhat, float3 khat, float3 v) {
      return ihat * v.x + jhat * v.y + khat * v.z;
    }
};


struct Model {
  vector<float3> vertices; // List of vertices
  vector<float3> cols; // List of vertex colors

  Transform transform;
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
 * Helper function to check if a point is on the right side of a line segment.
 * @param a The first point of the line segment.
 * @param b The second point of the line segment.
 * @param p The point to check.
 * @return True if the point is on the right side, false otherwise.
 */
bool point_on_rightside(float2 a, float2 b, float2 p) {
  float2 ap = p - a;
  float2 abPerp = float2::get_purpendicular(b - a);

  return float2::dot(ap, abPerp) >= 0;
}


/**
 * Helper function to check if a point is inside a triangle using barycentric coordinates.
 * @param a The first vertex of the triangle.
 * @param b The second vertex of the triangle.
 * @param c The third vertex of the triangle.
 * @param p The point to check.
 * @return True if the point is inside the triangle, false otherwise.
 */
bool point_in_triangle(const float2& a, const float2& b, const float2& c, const float2& p) {
  bool side_ab = point_on_rightside(a, b, p);
  bool side_bc = point_on_rightside(b, c, p);
  bool side_ca = point_on_rightside(c, a, p);

  return side_ab && side_bc && side_ca;
}


/**
 * Converts world coordinates of a vertex to screen coordinates.
 * @param vertex The vertex in world coordinates.
 * @param transform The transformation to apply to the vertex.
 * @param dim The dimensions of the screen (width, height).
 * @return The screen coordinates of the vertex as a float2.
 */
float2 vertex_to_screen(const float3 &vertex, Transform transform, float2 dim) {
  float3 vertex_world = transform.to_world_point(vertex);
  float fov = 1.0472; // 60 degrees in radians

  float screen_height_world = tan(fov / 2) * 2;
  float pixels_per_world_unit = dim.y / screen_height_world / vertex_world.z;

  float2 screen_pos;
  screen_pos.x = vertex_world.x * pixels_per_world_unit;
  screen_pos.y = vertex_world.y * pixels_per_world_unit;

  return (dim / 2) + screen_pos;
}


/**
 * Reads vertices and face indices from an OBJ file.
 * Only supports 'v' (vertex) and 'f' (face) lines.
 * Returns a vector of float3 containing all vertices used in faces (in face order).
 */
vector<float3> read_obj(const string& filename) {
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


/**
 * Renders a model to the pixel buffer in the RenderState.
 * @param model The model to render.
 * @param state The render state containing pixel buffer and dimensions.
 */
void render_model(Model &model, RenderState &state) {
  // Render each triangle in the model
  for (size_t i = 0; i < model.vertices.size(); i += 3) {
    float2 a = vertex_to_screen(model.vertices[i], model.transform, state.dim());
    float2 b = vertex_to_screen(model.vertices[i + 1], model.transform, state.dim());
    float2 c = vertex_to_screen(model.vertices[i + 2], model.transform, state.dim());

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
  Model model;

  // load vertices
  model.vertices = read_obj("cube.obj");
  int num_triangles = model.vertices.size() / 3;

  // Generate the triangles and their colors
  model.cols.resize(num_triangles);
  for (int i = 0; i < model.vertices.size() / 3; ++i) {
    model.cols[i] = random_colour();
  }

  // rotate the cube
  model.transform.yaw = 0.6f;
  model.transform.pitch = 0.3f;
  model.transform.position = float3(0, 0, 5); // Position the cube in front of the camera 
  
  render_model(model, state);
}


int main(void) {
  srand(time(NULL));

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