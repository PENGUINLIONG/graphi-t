// # 3D Mesh utilities
// @PENGUINLIONG
#pragma once
#include <vector>
#include <string>
#include "gft/vmath.hpp"

namespace liong {
namespace mesh {

struct Mesh {
  std::vector<vmath::float3> poses;
  std::vector<vmath::float2> uvs;
  std::vector<vmath::float3> norms;
};

extern bool try_parse_obj(const std::string& obj, Mesh& mesh);
extern Mesh load_obj(const char* path);

} // namespace mesh
} // namespace liong
