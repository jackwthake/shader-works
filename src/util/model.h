#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>
#include "maths.h"

struct Model {
  std::vector<float3> vertices; // List of vertices
  std::vector<float3> cols; // List of vertex colors

  Transform transform;
};


std::vector<float3> read_obj(const std::string& content);

#endif // MODEL_H