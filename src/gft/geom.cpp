#include "gft/geom.hpp"

namespace liong {
namespace geom {

using namespace glm;

bool contains_point_aabb(const Aabb& aabb, const vec3& point) {
  return
    aabb.min.x <= point.x &&
    aabb.min.y <= point.y &&
    aabb.min.z <= point.z &&
    aabb.max.x >= point.x &&
    aabb.max.y >= point.y &&
    aabb.max.z >= point.z;
}
bool contains_point_sphere(const Sphere& sphere, const vec3& point) {
  return (point - sphere.p).length() <= sphere.r;
}
bool contains_point_tet(const Tetrahedron& tet, const vec3& point) {
  vec3 p = (tet.a + tet.b + tet.c + tet.d) * 0.5f;
  vec3 v = point - p;
  vec3 pa = tet.a - p;
  vec3 pb = tet.b - p;
  vec3 pc = tet.c - p;
  vec3 pd = tet.d - p;
  float al = dot(v, pa) / dot(pa, pa);
  float bl = dot(v, pb) / dot(pb, pb);
  float cl = dot(v, pc) / dot(pc, pc);
  float dl = dot(v, pd) / dot(pd, pd);
  if (al > 1.0f || bl > 1.0f || cl > 1.0f || dl > 1.0f) {
    return false;
  }
  return true;
}



bool intersect_aabb(const Aabb& aabb1, const Aabb& aabb2) {
  return
    aabb1.min.x <= aabb2.max.x ||
    aabb1.min.y <= aabb2.max.y ||
    aabb1.min.z <= aabb2.max.z ||
    aabb1.max.x >= aabb2.min.x ||
    aabb1.max.y >= aabb2.min.y ||
    aabb1.max.z >= aabb2.min.z;
}



void split_tet2tris(const Tetrahedron& tet, std::vector<Triangle>& out) {
  Triangle t0 { tet.a, tet.b, tet.c };
  Triangle t1 { tet.a, tet.b, tet.d };
  Triangle t2 { tet.a, tet.c, tet.d };
  Triangle t3 { tet.b, tet.c, tet.d };

  out.emplace_back(std::move(t0));
  out.emplace_back(std::move(t1));
  out.emplace_back(std::move(t2));
  out.emplace_back(std::move(t3));
}

void split_aabb2tets(const Aabb& aabb, std::vector<Tetrahedron>& out) {
  // Any cube can be split into five tetrahedra and in this function we split
  // the AABB in a hard-coded pattern for simplicity.
  //
  //      A___________B
  //      /|         /|
  //     / |        / |
  //   D/__|______C/  |   Y
  //    |  |       |  |   |
  //    | E|_______|_F|   |____X
  //    | /        | /   /
  //    |/_________|/   Z
  //    H          G
  //
  // ABDE, BCDG, DEGH, BEGF, BDEG

  glm::vec3 a(aabb.min.x, aabb.max.y, aabb.min.z);
  glm::vec3 b(aabb.max.x, aabb.max.y, aabb.min.z);
  glm::vec3 c(aabb.max.x, aabb.max.y, aabb.max.z);
  glm::vec3 d(aabb.min.x, aabb.max.y, aabb.max.z);
  glm::vec3 e(aabb.min.x, aabb.min.y, aabb.min.z);
  glm::vec3 f(aabb.max.x, aabb.min.y, aabb.min.z);
  glm::vec3 g(aabb.max.x, aabb.min.y, aabb.max.z);
  glm::vec3 h(aabb.min.x, aabb.min.y, aabb.max.z);

  Tetrahedron t0 { a, b, d, e };
  Tetrahedron t1 { b, c, d, g };
  Tetrahedron t2 { d, e, g, h };
  Tetrahedron t3 { b, e, g, f };
  Tetrahedron t4 { b, d, e, g };

  out.emplace_back(std::move(t0));
  out.emplace_back(std::move(t1));
  out.emplace_back(std::move(t2));
  out.emplace_back(std::move(t3));
  out.emplace_back(std::move(t4));
}

void subdivide_aabb(
  const Aabb& aabb,
  const glm::uvec3& nslice,
  std::vector<Aabb>& out
) {
  glm::vec3 size = aabb.size() / glm::vec3(nslice);

  for (uint32_t z = 0; z < nslice.z; ++z) {
    float z_min = aabb.min.z + z * size.z;
    float z_max = z + 1 == nslice.y ?
      aabb.max.z : aabb.min.z + (z + 1) * size.z;
    for (uint32_t y = 0; y < nslice.y; ++y) {
      float y_min = aabb.min.y + y * size.y;
      float y_max = y + 1 == nslice.y ?
        aabb.max.y : aabb.min.y + (y + 1) * size.y;
      for (uint32_t x = 0; x < nslice.x; ++x) {
        float x_min = aabb.min.x + x * size.x;
        float x_max = x + 1 == nslice.x ?
          aabb.max.x : aabb.min.x + (x + 1) * size.x;

        Aabb aabb2 {};
        aabb2.min = glm::vec3(x_min, y_min, z_min);
        aabb2.max = glm::vec3(x_max, y_max, z_max);
      }
    }
  }
}

void tile_aabb_ceil(
  const Aabb& aabb,
  const glm::vec3& tile_size,
  std::vector<Aabb>& out
) {
  glm::uvec3 nslice(glm::ceil(aabb.size() / tile_size));
  glm::vec3 size2 = glm::vec3(nslice) * tile_size;
  Aabb aabb2 = Aabb::from_center_size(aabb.center(), size2);
  subdivide_aabb(aabb, nslice, out);
}

} // namespace geom
} // namespace liong
