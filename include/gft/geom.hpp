// Geometry algorithms all in right-hand-side systems.
// @PENGUINLIONG
#pragma once
#include <vector>
#include "glm/glm.hpp"

namespace liong {
namespace geom {

struct Ray {
  glm::vec3 p;
  glm::vec3 v;
};
struct Triangle {
  glm::vec3 a;
  glm::vec3 b;
  glm::vec3 c;
};
struct Aabb {
  glm::vec3 min;
  glm::vec3 max;

  constexpr glm::vec3 center() const {
    return max * 0.5f + min * 0.5f;
  }
  constexpr glm::vec3 size() const {
    return max - min;
  }

  static constexpr Aabb from_min_max(
    const glm::vec3& min,
    const glm::vec3& max
  ) {
    return Aabb { min, max };
  }
  static constexpr Aabb from_center_size(
    const glm::vec3& center,
    const glm::vec3& size
  ) {
    return Aabb { center - 0.5f * size, center + 0.5f * size };
  }
  static constexpr Aabb from_points(const glm::vec3* points, size_t npoint) {
    Aabb out {};
    glm::vec3 min {
      std::numeric_limits<float>::infinity(),
      std::numeric_limits<float>::infinity(),
      std::numeric_limits<float>::infinity(),
    };
    glm::vec3 max {
      -std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity()
    };
    for (size_t i = 0; i < npoint; ++i) {
      const glm::vec3& point = points[i];
      min = glm::min(point, min);
      max = glm::max(point, max);
    }
    return Aabb::from_min_max(min, max);
  }
  static constexpr Aabb from_points(const std::vector<glm::vec3>& points) {
    return from_points(points.data(), points.size());
  }
};
struct Sphere {
  glm::vec3 p;
  float r;
};
struct Tetrahedron {
  glm::vec3 a;
  glm::vec3 b;
  glm::vec3 c;
  glm::vec3 d;
};
struct Plane {
  glm::vec3 n;
  glm::vec3 u;
  glm::vec3 v;
};

enum Facing {
  L_FACING_NONE,
  L_FACING_FRONT,
  L_FACING_BACK,
  L_FACING_FRONT_AND_BACK,
};



extern bool raycast_tri(
  const Ray& ray,
  const Triangle& tri,
  float& t,
  glm::vec2& bary
);
extern bool raycast_aabb(const Ray& ray, const Aabb& aabb, float& t);
extern bool raycast_sphere(const Ray& ray, const Sphere& sphere, float& t);
extern bool raycast_tet(const Ray& ray, const Tetrahedron& tet, float& t);

extern bool contains_point_aabb(const Aabb& aabb, const glm::vec3& point);
extern bool contains_point_sphere(const Sphere& sphere, const glm::vec3& point);
extern bool contains_point_tetra(const Tetrahedron& tet, const glm::vec3& point, glm::vec4& bary);

extern bool intersect_tri(const Triangle& tri1, const Triangle& tri2);
extern bool intersect_aabb_tri(const Triangle& tri, const Aabb& aabb);
extern bool intersect_aabb(const Aabb& aabb1, const Aabb& aabb2);

extern void split_tetra2tris(const Tetrahedron& tet, std::vector<Triangle>& tris);
extern void split_aabb2tetras(const Aabb& aabb, std::vector<Tetrahedron>& tets);

extern void subdivide_aabb(
  const Aabb& aabb,
  const glm::uvec3& nslice,
  std::vector<Aabb>& out
);
extern void tile_aabb_ceil(
  const Aabb& aabb,
  const glm::vec3& tile_size,
  std::vector<Aabb>& out
);

} // namespace geom
} // namespace liong
