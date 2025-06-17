#include "model.h"

#include <sstream>

using namespace std;

/**
 * Read in an OBJ file
 * @param content the contents of the desired obj file
 * @returns A vector of vertices ordered by their faces
 */
vector<float3> read_obj(const string& content) {
  vector<float3> vertices, out_vertices;
  istringstream stream(content);
  string line;
  
  while (getline(stream, line)) {
    if (line.empty()) continue;

    // get first char of each line
    // useful for telling us which type of data we are reading
    istringstream iss(line);
    string prefix;
    iss >> prefix;

    if (prefix == "v") { // vertices
      float x, y, z;
      if (iss >> x >> y >> z) {
        vertices.emplace_back(x, y, z);
      }
    } else if (prefix == "f") { // faces
      int count = 0;
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
  
  return out_vertices;
}