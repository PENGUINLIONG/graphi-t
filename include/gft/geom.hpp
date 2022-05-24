// Geometry algorithms all in right-hand-side systems.
// @PENGUINLIONG
#pragma once
#include "glm/glm.hpp"

namespace liong {
namespace geom {

struct Ray {
  glm::vec3 p;
  glm::vec3 v;
};
struct Triangle {
  glm::vec3 p;
  glm::vec3 u;
  glm::vec3 v;
};
struct Aabb {
  glm::vec3 min;
  glm::vec3 max;
};
struct Sphere {
  glm::vec3 p;
  float r;
};
struct Tetrahedron {
  glm::vec3 p;
  glm::vec3 a;
  glm::vec3 b;
  glm::vec3 c;
  glm::vec3 d;
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
extern bool contains_point_tet(const Tetrahedron& tet, const glm::vec3& point);

extern bool intersect_tri(const Triangle& tri1, const Triangle& tri2);
extern bool intersect_aabb_tri(const Triangle& tri, const Aabb& aabb);
extern bool intersect_aabb(const Aabb& aabb1, const Aabb& aabb2);

} // namespace geom
} // namespace liong
