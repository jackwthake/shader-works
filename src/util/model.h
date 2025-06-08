#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>
#include "vec.h"

std::vector<float3> read_obj(const std::string& content);

#endif // MODEL_H