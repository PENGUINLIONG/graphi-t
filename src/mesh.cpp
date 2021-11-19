// # 3D Mesh utilities
// @PENGUINLIONG
#include "vmath.hpp"
#include "mesh.hpp"
#include "assert.hpp"

namespace liong {
namespace mesh {

using namespace vmath;

Mesh load_obj(const char* path) {
  Mesh out {};

  auto txt = liong::util::load_text(path);
  auto lines = liong::util::split('\n', txt);

  for (const auto& line : lines) {
    auto segs = liong::util::split(' ', liong::util::trim(line));
    const auto& cmd = segs[0];

    if (cmd == "v") {
      // Vertex.
      liong::assert(segs.size() == 4, "vertex coordinate field must have 3 components");
      float4 position {
        (float)std::atof(segs[1].c_str()),
        (float)std::atof(segs[2].c_str()),
        (float)std::atof(segs[3].c_str()),
        1.0f,
      };
      out.positions.emplace_back(std::move(position));
    } else if (cmd == "vt") {
      liong::assert(segs.size() == 3, "texcoord field must have 2 components");
      // UV coordinates.
      float2 uv {
        (float)std::atof(segs[1].c_str()),
        (float)std::atof(segs[2].c_str()),
      };
      out.uvs.emplace_back(std::move(uv));
    } else if (cmd == "f") {
      liong::assert(segs.size() == 4, "face field must have 3 indices");
      auto idx1 = liong::util::split('/', segs[1]);
      auto idx2 = liong::util::split('/', segs[2]);
      auto idx3 = liong::util::split('/', segs[3]);
      liong::assert(idx1[0] == idx1[1], "position indices must match texcoord indices");
      uint16_t i1 = std::atoi(idx1[0].c_str());
      uint16_t i2 = std::atoi(idx2[0].c_str());
      uint16_t i3 = std::atoi(idx3[0].c_str());
      out.indices.emplace_back(i1);
      out.indices.emplace_back(i2);
      out.indices.emplace_back(i3);
    }
  }
  return out;
}


} // namespace mesh
} // namespace liong
