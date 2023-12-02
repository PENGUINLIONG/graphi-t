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

struct MeshGpu {
  const uint32_t nvert;
  scoped::Buffer poses;
  scoped::Buffer uvs;
  scoped::Buffer norms;

  MeshGpu(
    const scoped::Context& ctxt,
    uint32_t nvert,
    bool streaming = true,
    bool gc = true
  );
  MeshGpu(const scoped::Context& ctxt, const mesh::Mesh& mesh, bool gc = true);

  void write(const mesh::Mesh& mesh);
};
struct IndexedMeshGpu {
  MeshGpu mesh;
  const uint32_t ntri;
  scoped::Buffer idxs;

  IndexedMeshGpu(
    const scoped::Context& ctxt,
    uint32_t nvert,
    uint32_t ntri,
    bool streaming = true,
    bool gc = true
  );
  IndexedMeshGpu(
    const scoped::Context& ctxt,
    const mesh::IndexedMesh& idxmesh,
    bool gc = true
  );

  void write(const mesh::IndexedMesh& idxmesh);
};
struct SkinnedMeshGpu {
  scoped::Context ctxt;
  IndexedMeshGpu idxmesh;
  const uint32_t nbone;

  scoped::Buffer rest_poses;
  scoped::Buffer ibones;
  scoped::Buffer bone_weights;

  scoped::Buffer bone_mats;

  mesh::Skinning skinning;
  mesh::SkeletalAnimationCollection skel_anims;

  SkinnedMeshGpu(
    const scoped::Context& ctxt,
    uint32_t nvert,
    uint32_t ntri,
    uint32_t nbone,
    bool streaming = true,
    bool gc = true
  );
  SkinnedMeshGpu(
    const scoped::Context& ctxt,
    const mesh::SkinnedMesh& skinmesh,
    bool gc = true
  );

  void write(const mesh::SkinnedMesh& skinmesh);

  scoped::Invocation animate(const std::string& anim_name, float tick);
  scoped::Invocation animate(float tick);
};

struct TextureGpu {
  scoped::Context ctxt;
  scoped::Buffer stage_buf;
  scoped::Image tex;

  TextureGpu(
    const scoped::Context& ctxt,
    uint32_t width,
    uint32_t height,
    bool streaming = true,
    bool gc = true
  );
  TextureGpu(
    const scoped::Context& ctxt,
    uint32_t width,
    uint32_t height,
    const std::vector<uint32_t>& pxs,
    bool gc = true
  );

  void write(const std::vector<uint32_t>& pxs);
};

struct RendererInvocationDetail {
  std::unique_ptr<scoped::RenderPassInvocationBuilder> rpib;
};
struct Renderer {
  scoped::Context ctxt;
  scoped::RenderPass pass;
  scoped::DepthImage zbuf_img;
  scoped::Task lit_task;
  scoped::Task wireframe_task;
  scoped::Task point_cloud_task;

  scoped::TextureGpu default_tex;

  uint32_t width;
  uint32_t height;
  glm::vec3 camera_pos;
  glm::vec3 model_pos;
  glm::vec3 light_dir;
  glm::vec3 ambient;
  glm::vec3 albedo;

  std::unique_ptr<scoped::RenderPassInvocationBuilder> rpib;

  Renderer(const scoped::Context& ctxt, uint32_t width, uint32_t height);

  glm::mat4 get_model2world() const;
  glm::mat4 get_world2view() const;

  void set_camera_pos(const glm::vec3& x);
  void set_model_pos(const glm::vec3& x);

  Renderer& begin_frame(const scoped::Image& render_target_img);
  scoped::Invocation end_frame();

  Renderer& is_timed(bool is_timed = true);

  Renderer& draw_mesh(const mesh::Mesh& mesh);

  Renderer& draw_idxmesh(
    const scoped::IndexedMeshGpu& idxmesh,
    const scoped::TextureGpu& tex
  );
  Renderer& draw_idxmesh(const scoped::IndexedMeshGpu& idxmesh);

  Renderer& draw_idxmesh(
    const mesh::IndexedMesh& idxmesh,
    const scoped::TextureGpu& tex
  );
  Renderer& draw_idxmesh(const mesh::IndexedMesh& idxmesh);

  Renderer& draw_mesh_wireframe(
    const mesh::Mesh& mesh,
    const std::vector<glm::vec3>& colors
  );
  Renderer& draw_mesh_wireframe(const mesh::Mesh& mesh, const glm::vec3& color);
  Renderer& draw_mesh_wireframe(const mesh::Mesh& mesh);

  Renderer& draw_idxmesh_wireframe(
    const mesh::IndexedMesh& idxmesh,
    const std::vector<glm::vec3>& colors
  );
  Renderer& draw_idxmesh_wireframe(
    const mesh::IndexedMesh& idxmesh,
    const glm::vec3& color
  );
  Renderer& draw_idxmesh_wireframe(const mesh::IndexedMesh& idxmesh);

  Renderer& draw_point_cloud(
    const mesh::PointCloud& point_cloud,
    const std::vector<glm::vec3>& colors
  );
  Renderer& draw_point_cloud(
    const mesh::PointCloud& point_cloud,
    const glm::vec3& colors
  );
  Renderer& draw_point_cloud(const mesh::PointCloud& point_cloud);
};

struct RenderInvocationBuilder {
  RenderInvocationBuilder(
    const Renderer& renderer,
    const std::string& label = ""
  );
};

} // namespace scoped
} // namespace HAL_IMPL_NAMESPACE
} // namespace liong
