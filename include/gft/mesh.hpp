// # 3D Mesh utilities
// @PENGUINLIONG
#pragma once
#include <vector>
#include <string>
#include "glm/glm.hpp"

namespace liong {
namespace mesh {

struct Mesh {
  std::vector<glm::vec3> poses;
  std::vector<glm::vec2> uvs;
  std::vector<glm::vec3> norms;
};

extern bool try_parse_obj(const std::string& obj, Mesh& mesh);
extern Mesh load_obj(const char* path);



struct IndexedMesh {
  Mesh mesh;
  std::vector<glm::uvec3> idxs;
};

extern IndexedMesh mesh2idxmesh(const Mesh& mesh);



struct PointCloud {
  std::vector<glm::vec3> poses;
};



struct Aabb {
  glm::vec3 min;
  glm::vec3 max;

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
  Aabb(std::initializer_list<glm::vec3> pts) : Aabb() {
    for (const auto& pt : pts) {
      push(pt);
    }
  }

  static Aabb from_min_max(
    const glm::vec3& min,
    const glm::vec3& max
  ) {
    return Aabb { min, max };
  }
  static Aabb from_center_size(
    const glm::vec3& center,
    const glm::vec3& size
  ) {
    glm::vec3 min = center - size * 0.5f;
    glm::vec3 max = center + size * 0.5f;
    return Aabb { min, max };
  }

  glm::vec3 center() const {
    return (min + max) * 0.5f;
  }
  glm::vec3 size() const {
    return max - min;
  }

  inline void push(const glm::vec3& v) {
    min = glm::min(v, min);
    max = glm::max(v, max);
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

  inline Aabb align(const glm::vec3& interval) const {
    glm::vec3 size2 = glm::ceil(size() / interval) * interval;
    return Aabb::from_center_size(center(), size2);
  }
};



struct Bin {
  Aabb aabb;
  std::vector<uint32_t> iprims;
};
struct BinGrid {
  std::vector<float> grid_lines_x;
  std::vector<float> grid_lines_y;
  std::vector<float> grid_lines_z;
  std::vector<Bin> bins;
};

extern BinGrid bin_point_cloud(
  const Aabb& aabb,
  const glm::uvec3& grid_res,
  const PointCloud& point_cloud
);
extern BinGrid bin_mesh(
  const Aabb& aabb,
  const glm::uvec3& grid_res,
  const Mesh& mesh
);
extern BinGrid bin_idxmesh(
  const Aabb& aabb,
  const glm::uvec3& grid_res,
  const IndexedMesh& idxmesh
);

} // namespace mesh
} // namespace liong
