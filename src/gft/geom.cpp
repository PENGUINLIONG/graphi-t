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
  vec3 v = point - tet.p;
  float al = dot(v, tet.a) / dot(tet.a, tet.a);
  float bl = dot(v, tet.b) / dot(tet.b, tet.b);
  float cl = dot(v, tet.c) / dot(tet.c, tet.c);
  float dl = dot(v, tet.d) / dot(tet.d, tet.d);
  if (al > 1.0f || bl > 1.0f || cl > 1.0f || dl > 1.0f) {
    return false;
  }
  return true;
}

} // namespace geom
} // namespace liong
