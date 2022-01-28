// # 3D Mesh utilities
// @PENGUINLIONG
#pragma once
#include <vector>
#include <string>
#include "vmath.hpp"

namespace liong {
namespace mesh {

struct Mesh {
  std::vector<vmath::float3> positions;
  std::vector<vmath::float2> uvs;
  std::vector<uint16_t> indices;
};

extern Mesh parse_obj(const std::string& obj);
extern Mesh load_obj(const char* path);

} // namespace mesh
} // namespace liong
