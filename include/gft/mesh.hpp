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



struct IndexedMesh {
  Mesh mesh;
  std::vector<vmath::uint3> idxs;
};

extern IndexedMesh mesh2idxmesh(const Mesh& mesh);



struct PointCloud {
  std::vector<vmath::float3> poses;
};



struct Aabb {
  vmath::float3 min;
  vmath::float3 max;

  Aabb() :
    min(
      std::numeric_limits<float>::infinity(),
      std::numeric_limits<float>::infinity(),
      std::numeric_limits<float>::infinity()
    ),
    max(
      -std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity()
    ) {}
  Aabb(std::initializer_list<vmath::float3> pts) : Aabb() {
    for (const auto& pt : pts) {
      push(pt);
    }
  }

  inline void push(const vmath::float3& v) {
    min = vmath::min(v, min);
    max = vmath::max(v, max);
  }

  inline bool intersects_with(const Aabb& aabb) const {
    return
      min.x <= aabb.max.x ||
      min.y <= aabb.max.y ||
      min.z <= aabb.max.z ||
      max.x >= aabb.min.x ||
      max.y >= aabb.min.y ||
      max.z >= aabb.min.z;
  }
};



struct Bin {
  Aabb aabb;
  std::vector<size_t> iprims;
};
struct BinGrid {
  std::vector<float> grid_lines_x;
  std::vector<float> grid_lines_y;
  std::vector<float> grid_lines_z;
  std::vector<Bin> bins;
};

} // namespace mesh
} // namespace liong
