// # 3D Mesh utilities
// @PENGUINLIONG
#pragma once
#include <vector>
#include <string>
#include "glm/glm.hpp"
#include "gft/geom.hpp"

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



struct Bin {
  geom::Aabb aabb;
  std::vector<uint32_t> iprims;
};
struct BinGrid {
  std::vector<float> grid_lines_x;
  std::vector<float> grid_lines_y;
  std::vector<float> grid_lines_z;
  std::vector<Bin> bins;
};

extern BinGrid bin_point_cloud(
  const geom::Aabb& aabb,
  const glm::uvec3& grid_res,
  const PointCloud& point_cloud
);
extern BinGrid bin_mesh(
  const geom::Aabb& aabb,
  const glm::uvec3& grid_res,
  const Mesh& mesh
);
extern BinGrid bin_idxmesh(
  const geom::Aabb& aabb,
  const glm::uvec3& grid_res,
  const IndexedMesh& idxmesh
);

} // namespace mesh
} // namespace liong
