#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>
#include "maths.h"


struct Model {
  std::vector<float3> vertices; // List of vertices
  std::vector<float3> cols; // List of vertex colors
  std::vector<float2> uvs; // texture coordinates

  int texture_atlas_id;
  Transform transform;

  Model() : texture_atlas_id(-1) {}
};

#endif // MODEL_H