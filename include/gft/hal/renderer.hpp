// # A simple renderer for debugging.
// @PENGUINLIONG
#pragma once
#include "gft/hal/scoped-hal.hpp"
#include "gft/mesh.hpp"

#ifndef HAL_IMPL_NAMESPACE
static_assert(false, "please specify the implementation namespace (e.g. `vk`)");
#endif

namespace liong {
namespace HAL_IMPL_NAMESPACE {
namespace scoped {

struct Renderer {
  scoped::Context ctxt;
  scoped::RenderPass pass;
  scoped::DepthImage zbuf_img;
  scoped::Task lit_task;
  scoped::Task wireframe_task;
  scoped::Task point_cloud_task;

  scoped::Image default_tex_img;

  uint32_t width;
  uint32_t height;
  glm::vec3 camera_pos;
  glm::vec3 model_pos;
  glm::vec3 light_dir;
  glm::vec3 ambient;
  glm::vec3 albedo;

  std::unique_ptr<scoped::RenderPassInvocationBuilder> rpib;

  Renderer(
    const scoped::Context& ctxt,
    uint32_t width,
    uint32_t height
  );

  glm::mat4 get_model2world() const;
  glm::mat4 get_world2view() const;

  void set_camera_pos(const glm::vec3& x);
  void set_model_pos(const glm::vec3& x);

  Renderer& begin_frame(const scoped::Image& render_target_img);
  void end_frame();

  Renderer& draw_mesh(const mesh::Mesh& mesh);
  Renderer& draw_idxmesh(const mesh::IndexedMesh& idxmesh);
  Renderer& draw_mesh_wireframe(const mesh::Mesh& mesh, const std::vector<glm::vec3>& colors);
  Renderer& draw_mesh_wireframe(const mesh::Mesh& mesh, const glm::vec3& color);
  Renderer& draw_mesh_wireframe(const mesh::Mesh& mesh);
  Renderer& draw_idxmesh_wireframe(const mesh::IndexedMesh& idxmesh, const std::vector<glm::vec3>& colors);
  Renderer& draw_idxmesh_wireframe(const mesh::IndexedMesh& idxmesh, const glm::vec3& color);
  Renderer& draw_idxmesh_wireframe(const mesh::IndexedMesh& idxmesh);
  Renderer& draw_point_cloud(const mesh::PointCloud& point_cloud, const std::vector<glm::vec3>& colors);
  Renderer& draw_point_cloud(const mesh::PointCloud& point_cloud, const glm::vec3& colors);
  Renderer& draw_point_cloud(const mesh::PointCloud& point_cloud);
  //Renderer& draw_idxmesh(const mesh::IndexedMesh& idxmesh);
};

struct RenderInvocationBuilder {
  RenderInvocationBuilder(const Renderer& renderer, const std::string& label = "");
};

} // namespace scoped
} // HAL_IMPL_NAMESPACE
} // namespace liong
