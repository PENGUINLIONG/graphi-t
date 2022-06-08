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

  scoped::Image default_tex_img;

  uint32_t width;
  uint32_t height;
  glm::vec3 camera_pos;
  glm::vec3 light_dir;
  glm::vec3 ambient;
  glm::vec3 albedo;

  std::unique_ptr<scoped::RenderPassInvocationBuilder> rpib;

  Renderer(
    const scoped::Context& ctxt,
    uint32_t width,
    uint32_t height
  );

  Renderer& begin_frame(const scoped::Image& render_target_img);
  void end_frame();

  Renderer& draw_mesh(const mesh::Mesh& mesh);
  Renderer& draw_mesh_wireframe(const mesh::Mesh& mesh, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 0.0f));
  //Renderer& draw_idxmesh(const mesh::IndexedMesh& idxmesh);
};

struct RenderInvocationBuilder {
  RenderInvocationBuilder(const Renderer& renderer, const std::string& label = "");
};

} // namespace scoped
} // HAL_IMPL_NAMESPACE
} // namespace liong
