#include "gft/vk.hpp"
#include "gft/hal/renderer.hpp"
#include "gft/glslang.hpp"
#include "glm/glm.hpp"
#include "glm/ext.hpp"


namespace liong {
namespace vk {
namespace scoped {

MeshGpu::MeshGpu(const scoped::Context& ctxt, uint32_t nvert, bool streaming, bool gc) : nvert(nvert) {
  poses = ctxt.build_buf()
    .size(nvert * sizeof(glm::vec4))
    .vertex()
    .storage()
    .host_access(streaming ? L_MEMORY_ACCESS_WRITE_BIT : 0)
    .build(gc);

  uvs = ctxt.build_buf()
    .size(nvert * sizeof(glm::vec2))
    .storage()
    .host_access(streaming ? L_MEMORY_ACCESS_WRITE_BIT : 0)
    .build(gc);

  norms = ctxt.build_buf()
    .size(nvert * sizeof(glm::vec4))
    .storage()
    .host_access(streaming ? L_MEMORY_ACCESS_WRITE_BIT : 0)
    .build(gc);
}
MeshGpu::MeshGpu(const scoped::Context& ctxt, const mesh::Mesh& mesh, bool gc) :
  MeshGpu(ctxt, mesh.poses.size(), gc)
{
  write(mesh);
}
void MeshGpu::write(const mesh::Mesh& mesh) {
  L_ASSERT(nvert == mesh.poses.size());
  L_ASSERT(nvert == mesh.uvs.size());
  L_ASSERT(nvert == mesh.norms.size());
  poses.map_write().write_aligned(mesh.poses, sizeof(glm::vec4));
  uvs.map_write().write_aligned(mesh.uvs, sizeof(glm::vec2));
  norms.map_write().write_aligned(mesh.norms, sizeof(glm::vec4));
}



IndexedMeshGpu::IndexedMeshGpu(
  const scoped::Context& ctxt,
  uint32_t nvert,
  uint32_t ntri,
  bool streaming,
  bool gc
) : mesh(ctxt, nvert, streaming, gc), ntri(ntri) {
  idxs = ctxt.build_buf()
    .size(ntri * sizeof(glm::uvec3))
    .index()
    .storage()
    .host_access(streaming ? L_MEMORY_ACCESS_WRITE_BIT : 0)
    .build(gc);
}
IndexedMeshGpu::IndexedMeshGpu(const scoped::Context& ctxt, const mesh::IndexedMesh& idxmesh, bool gc) :
  IndexedMeshGpu(ctxt, idxmesh.mesh.poses.size(), idxmesh.idxs.size(), gc)
{
  write(idxmesh);
}
void IndexedMeshGpu::write(const mesh::IndexedMesh& idxmesh) {
  L_ASSERT(ntri == idxmesh.idxs.size());
  mesh.write(idxmesh.mesh);
  idxs.map_write().write(idxmesh.idxs);
}



SkinnedMeshGpu::SkinnedMeshGpu(
  const scoped::Context& ctxt,
  uint32_t nvert,
  uint32_t ntri,
  uint32_t nbone,
  bool streaming,
  bool gc
) : ctxt(scoped::Context::borrow(ctxt)), idxmesh(ctxt, nvert, ntri, streaming, gc), nbone(nbone) {
  rest_poses = ctxt.build_buf()
    .size(nvert * sizeof(glm::vec4))
    .storage()
    .host_access(streaming ? L_MEMORY_ACCESS_WRITE_BIT : 0)
    .build();

  ibones = ctxt.build_buf()
    .size(nvert * sizeof(glm::uvec4))
    .storage()
    .host_access(streaming ? L_MEMORY_ACCESS_WRITE_BIT : 0)
    .build();

  bone_weights = ctxt.build_buf()
    .size(nvert * sizeof(glm::vec4))
    .storage()
    .host_access(streaming ? L_MEMORY_ACCESS_WRITE_BIT : 0)
    .build();

  bone_mats = ctxt.build_buf()
    .size(nvert * sizeof(glm::mat4))
    .storage()
    .host_access(streaming ? L_MEMORY_ACCESS_WRITE_BIT : 0)
    .build();
}
SkinnedMeshGpu::SkinnedMeshGpu(const scoped::Context& ctxt, const mesh::SkinnedMesh& skinmesh, bool gc) :
  SkinnedMeshGpu(ctxt, skinmesh.idxmesh.mesh.poses.size(), skinmesh.idxmesh.idxs.size(), skinmesh.skinning.bones.size(), gc)
{
  write(skinmesh);
}
scoped::Task create_animate_task(const scoped::Context& ctxt, uint32_t nvert) {
  std::string src = R"(
    #version 450 core
    layout(local_size_x_id=0, local_size_y_id=1, local_size_z_id=2) in;

    layout(binding=0) readonly buffer _0 { vec4 rest_poses[]; };
    layout(binding=1) readonly buffer _1 { uvec4 ibones[]; };
    layout(binding=2) readonly buffer _2 { vec4 bone_weights[]; };
    layout(binding=3) readonly buffer _3 { mat4 bone_mats[]; };
    layout(binding=4) writeonly buffer _4 { vec4 poses[]; };

    void main() {
      uvec3 global_id = gl_GlobalInvocationID;
      int i = int(global_id.x);
      if (i >= )" + std::to_string(nvert) + R"() return;

      vec4 rest_pos = vec4(rest_poses[i].xyz, 1.0f);

      uvec4 ibone = ibones[i];
      vec4 bone_weight = bone_weights[i];

      vec4 pos =
        bone_mats[ibone.x] * rest_pos * bone_weight.x +
        bone_mats[ibone.y] * rest_pos * bone_weight.y +
        bone_mats[ibone.z] * rest_pos * bone_weight.z +
        bone_mats[ibone.w] * rest_pos * bone_weight.w;

      poses[i] = pos;
    }
  )";

  auto art = glslang::compile_comp(src, "main");

  scoped::Task task = ctxt.build_comp_task()
    .comp(art.comp_spv)
    .rsc(L_RESOURCE_TYPE_STORAGE_BUFFER)
    .rsc(L_RESOURCE_TYPE_STORAGE_BUFFER)
    .rsc(L_RESOURCE_TYPE_STORAGE_BUFFER)
    .rsc(L_RESOURCE_TYPE_STORAGE_BUFFER)
    .rsc(L_RESOURCE_TYPE_STORAGE_BUFFER)
    .workgrp_size(64, 1, 1)
    .build(false);

  return task;
};
void SkinnedMeshGpu::write(const mesh::SkinnedMesh& skinmesh) {
  L_ASSERT(nbone == skinmesh.skinning.bones.size());
  idxmesh.write(skinmesh.idxmesh);
  rest_poses.map_write().write_aligned(skinmesh.idxmesh.mesh.poses, sizeof(glm::vec4));
  ibones.map_write().write(skinmesh.skinning.ibones);
  bone_weights.map_write().write(skinmesh.skinning.bone_weights);
  std::vector<glm::mat4> bone_mats_data(skinmesh.skinning.bones.size(), glm::identity<glm::mat4>());
  bone_mats.map_write().write(bone_mats_data);

  skinning = skinmesh.skinning;
  skel_anims = skinmesh.skel_anims;
}
scoped::Invocation SkinnedMeshGpu::animate(const std::string& anim_name, float tick) {
  std::vector<glm::mat4> bone_mats_data;
  skel_anims.get_skel_anim(anim_name).get_bone_transforms(skinning, tick, bone_mats_data);
  bone_mats.map_write().write(bone_mats_data);

  uint32_t nvert = idxmesh.mesh.nvert;
  std::string task_name = "__skinmesh_bone_animate" + std::to_string(nvert);
  scoped::Task task;
  if (!ctxt.try_get_global_task(task_name, task)) {
    task = ctxt.reg_global_task(task_name, create_animate_task(ctxt, nvert));
  }

  return task.build_comp_invoke()
    .rsc(rest_poses.view())
    .rsc(ibones.view())
    .rsc(bone_weights.view())
    .rsc(bone_mats.view())
    .rsc(idxmesh.mesh.poses.view())
    .workgrp_count(util::div_up(idxmesh.mesh.nvert, 64), 1, 1)
    .build();
}
scoped::Invocation SkinnedMeshGpu::animate(float tick) {
  return animate(skel_anims.skel_anims.front().name, tick);
}



TextureGpu::TextureGpu(
  const scoped::Context& ctxt,
  uint32_t width,
  uint32_t height,
  bool streaming,
  bool gc
) : ctxt(scoped::Context::borrow(ctxt)) {
  tex = ctxt.build_img()
    .width(width)
    .height(height)
    .fmt(fmt::L_FORMAT_R8G8B8A8_UNORM)
    .sampled()
    .storage()
    .build();
  stage_buf = ctxt.build_buf()
    .size(sizeof(uint32_t) * width * height)
    .streaming()
    .build();
}
TextureGpu::TextureGpu(
  const scoped::Context& ctxt,
  uint32_t width,
  uint32_t height,
  const std::vector<uint32_t>& pxs,
  bool gc
) : TextureGpu(ctxt, width, height, gc) {
  write(pxs);
}
void TextureGpu::write(const std::vector<uint32_t>& pxs) {
  const auto& tex_cfg = tex.cfg();
  L_ASSERT(pxs.size() == tex_cfg.width * tex_cfg.height);

  stage_buf.map_write().write(pxs);

  ctxt.build_trans_invoke()
    .src(stage_buf.view())
    .dst(tex.view())
    .build()
    .submit()
    .wait();
}



scoped::DepthImage create_zbuf(
  const scoped::Context& ctxt,
  uint32_t width,
  uint32_t height
) {
  scoped::DepthImage zbuf_img = ctxt.build_depth_img()
    .fmt(fmt::L_DEPTH_FORMAT_D32_SFLOAT)
    .attachment()
    .width(width)
    .height(height)
    .build();
  return zbuf_img;
}

scoped::RenderPass create_pass(
  const scoped::Context& ctxt,
  uint32_t width,
  uint32_t height
) {
  scoped::RenderPass pass = ctxt.build_pass()
    .clear_store_attm(fmt::L_FORMAT_B8G8R8A8_UNORM)
    .clear_store_attm(fmt::L_DEPTH_FORMAT_D32_SFLOAT)
    .width(width)
    .height(height)
    .build();
  return pass;
}
scoped::Task create_unlit_task(const scoped::RenderPass& pass, Topology topo) {
  const char* vert_src = R"(
    #version 460 core

    layout(location=0) in vec3 pos;
    layout(location=0) out vec4 v_color;

    layout(binding=0, std140) uniform Uniform {
      mat4 model2world;
      mat4 world2view;
    };
    layout(binding=1, std430) readonly buffer Colors {
      vec4 colors[];
    };

    void main() {
      v_color = colors[gl_VertexIndex];
      gl_Position = world2view * model2world * vec4(pos, 1.0);
    }
  )";
  const char* frag_src = R"(
    #version 460 core
    precision mediump float;

    layout(location=0) in highp vec4 v_color;
    layout(location=0) out vec4 scene_color;

    layout(binding=0, std140) uniform Uniform {
      mat4 model2world;
      mat4 world2view;
    };

    layout(binding=3) uniform sampler2D main_tex;

    void main() {
      scene_color = v_color;
    }
  )";

  auto art = glslang::compile_graph(vert_src, "main", frag_src, "main");

  auto wireframe_task = pass.build_graph_task()
    .vert(art.vert_spv)
    .frag(art.frag_spv)
    .rsc(L_RESOURCE_TYPE_UNIFORM_BUFFER)
    .rsc(L_RESOURCE_TYPE_STORAGE_BUFFER)
    .topo(topo)
    .build();
  return wireframe_task;
}
scoped::Task create_lit_task(const scoped::RenderPass& pass) {
  const char* vert_src = R"(
    #version 460 core

    layout(location=0) in vec3 pos;

    layout(location=0) out vec4 v_world_pos;
    layout(location=1) out vec2 v_uv;
    layout(location=2) out vec4 v_norm;

    layout(binding=0, std140) uniform Uniform {
      mat4 model2world;
      mat4 world2view;
      vec4 camera_pos;
      vec4 light_dir;
      vec4 ambient;
      vec4 albedo;
    };

    layout(binding=1, std430) readonly buffer Uvs {
      vec2 uvs[];
    };
    layout(binding=2, std430) readonly buffer Norms {
      vec4 norms[];
    };

    void main() {
      v_world_pos = model2world * vec4(pos, 1.0);
      v_uv = uvs[gl_VertexIndex];
      v_norm = model2world * norms[gl_VertexIndex];

      vec4 ndc_pos = world2view * v_world_pos;
      gl_Position = ndc_pos;
    }
  )";
  const char* frag_src = R"(
    #version 460 core
    precision mediump float;

    layout(location=0) in highp vec4 v_world_pos;
    layout(location=1) in highp vec2 v_uv;
    layout(location=2) in highp vec4 v_norm;

    layout(location=0) out vec4 scene_color;

    layout(binding=0, std140) uniform Uniform {
      mat4 model2world;
      mat4 world2view;
      vec4 camera_pos;
      vec4 light_dir;
      vec4 ambient;
      vec4 albedo;
    };

    layout(binding=3) uniform sampler2D main_tex;

    void main() {
      vec3 N = normalize(v_norm.xyz);
      vec3 V = normalize(camera_pos.xyz - v_world_pos.xyz);
      vec3 L = normalize(light_dir.xyz);
      vec3 H = normalize(V + L);
      float NoH = dot(N, H);

      vec3 diffuse = clamp(NoH, 0.0f, 1.0f) * texture(main_tex, v_uv).xyz;

      scene_color = vec4(albedo.xyz * diffuse.xyz + ambient.xyz, 1.0);
      scene_color = vec4(texture(main_tex, v_uv).xyz, 1.0);
    }
  )";

  auto art = glslang::compile_graph(vert_src, "main", frag_src, "main");

  auto lit_task = pass.build_graph_task()
    .vert(art.vert_spv)
    .frag(art.frag_spv)
    .rsc(L_RESOURCE_TYPE_UNIFORM_BUFFER)
    .rsc(L_RESOURCE_TYPE_STORAGE_BUFFER)
    .rsc(L_RESOURCE_TYPE_STORAGE_BUFFER)
    .rsc(L_RESOURCE_TYPE_SAMPLED_IMAGE)
    .topo(L_TOPOLOGY_TRIANGLE)
    .build();
  return lit_task;
}

scoped::TextureGpu create_default_tex(
  const scoped::Context& ctxt
) {
  std::vector<uint32_t> white_img_data(16, 0xffffffff);
  scoped::TextureGpu white_img(ctxt, 4, 4, white_img_data);
  return white_img;
}

Renderer::Renderer(
  const scoped::Context& ctxt,
  uint32_t width,
  uint32_t height
) :
  ctxt(scoped::Context::borrow(ctxt)),
  pass(create_pass(ctxt, width, height)),
  zbuf_img(create_zbuf(ctxt, width, height)),
  lit_task(create_lit_task(pass)),
  wireframe_task(create_unlit_task(pass, L_TOPOLOGY_TRIANGLE_WIREFRAME)),
  point_cloud_task(create_unlit_task(pass, L_TOPOLOGY_POINT)),
  default_tex(create_default_tex(ctxt)),
  width(width),
  height(height),
  camera_pos(0.0f, 0.0f, -10.0f),
  model_pos(0.0f, 0.0f, 0.0f),
  light_dir(0.5f, -1.0f, 1.0f),
  ambient(0.1f, 0.1f, 0.1f),
  albedo(1.0f, 0.1f, 1.0f),
  rpib(nullptr) {}

glm::mat4 Renderer::get_model2world() const {
  glm::mat4 model2world = glm::scale(glm::mat4x4(1.0f), glm::vec3(1.0f, -1.0f, -1.0f));
  return model2world;
}
glm::mat4 Renderer::get_world2view() const {
  glm::mat4 camera2view = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 1e-2f, 65534.0f);
  glm::mat4 world2camera = glm::lookAt(camera_pos, model_pos, glm::vec3(0.0f, 1.0f, 0.0f));
  return camera2view * world2camera;
}

void Renderer::set_camera_pos(const glm::vec3& x) {
  camera_pos = x;
}
void Renderer::set_model_pos(const glm::vec3& x) {
  model_pos = x;
}

Renderer& Renderer::begin_frame(const scoped::Image& render_target_img) {
  rpib = std::make_unique<RenderPassInvocationBuilder>(pass.build_pass_invoke());
  rpib->attm(render_target_img.view()).attm(zbuf_img.view());
  return *this;
}
scoped::Invocation Renderer::end_frame() {
  return rpib->build();
}

Renderer& Renderer::is_timed(bool is_timed) {
  rpib->is_timed(is_timed);
  return *this;
}

Renderer& Renderer::draw_mesh(const mesh::Mesh& mesh) {
  struct Uniform {
    glm::mat4 model2world;
    glm::mat4 world2view;
    glm::vec4 camera_pos;
    glm::vec4 light_dir;
    glm::vec4 ambient;
    glm::vec4 albedo;
  };
  Uniform u;
  u.model2world = get_model2world();
  u.world2view = get_world2view();
  u.camera_pos = glm::vec4(camera_pos, 1.0f);
  u.light_dir = glm::vec4(light_dir, 0.0f);
  u.ambient = glm::vec4(ambient, 1.0f);
  u.albedo = glm::vec4(albedo, 1.0f);

  scoped::Buffer uniform_buf = ctxt.build_buf()
    .uniform()
    .streaming_with(u)
    .build();

  scoped::Buffer poses_buf = ctxt.build_buf()
    .vertex()
    .streaming_with_aligned(mesh.poses, sizeof(glm::vec4))
    .build();

  scoped::Buffer uv_buf = ctxt.build_buf()
    .storage()
    .streaming_with(mesh.uvs)
    .build();

  scoped::Buffer norm_buf = ctxt.build_buf()
    .storage()
    .streaming_with_aligned(mesh.norms, sizeof(glm::vec4))
    .build();

  scoped::Invocation lit_invoke = lit_task.build_graph_invoke()
    .vert_buf(poses_buf.view())
    .nvert(mesh.poses.size())
    .rsc(uniform_buf.view())
    .rsc(uv_buf.view())
    .rsc(norm_buf.view())
    .rsc(default_tex.tex.view())
    .build();

  rpib->invoke(lit_invoke);
  return *this;
}

Renderer& Renderer::draw_idxmesh(const scoped::IndexedMeshGpu& idxmesh, const scoped::TextureGpu& tex) {
  struct Uniform {
    glm::mat4 model2world;
    glm::mat4 world2view;
    glm::vec4 camera_pos;
    glm::vec4 light_dir;
    glm::vec4 ambient;
    glm::vec4 albedo;
  };
  Uniform u;
  u.model2world = get_model2world();
  u.world2view = get_world2view();
  u.camera_pos = glm::vec4(camera_pos, 1.0f);
  u.light_dir = glm::vec4(light_dir, 0.0f);
  u.ambient = glm::vec4(ambient, 1.0f);
  u.albedo = glm::vec4(albedo, 1.0f);

  scoped::Buffer uniform_buf = ctxt.build_buf()
    .uniform()
    .streaming_with(u)
    .build();

  scoped::Invocation lit_invoke = lit_task.build_graph_invoke()
    .vert_buf(idxmesh.mesh.poses.view())
    .idx_buf(idxmesh.idxs.view())
    .idx_ty(L_INDEX_TYPE_UINT32)
    .nidx(idxmesh.ntri * 3)
    .rsc(uniform_buf.view())
    .rsc(idxmesh.mesh.uvs.view())
    .rsc(idxmesh.mesh.norms.view())
    .rsc(tex.tex.view())
    .build();

  rpib->invoke(lit_invoke);
  return *this;
}
Renderer& Renderer::draw_idxmesh(const scoped::IndexedMeshGpu& idxmesh) {
  return draw_idxmesh(idxmesh, default_tex);
}
Renderer& Renderer::draw_idxmesh(const mesh::IndexedMesh& idxmesh, const scoped::TextureGpu& tex) {
  scoped::IndexedMeshGpu idxmesh2(ctxt, idxmesh);
  return draw_idxmesh(idxmesh2, tex);
}
Renderer& Renderer::draw_idxmesh(const mesh::IndexedMesh& idxmesh) {
  return draw_idxmesh(idxmesh, default_tex);
}

Renderer& Renderer::draw_mesh_wireframe(const mesh::Mesh& mesh, const std::vector<glm::vec3>& colors) {
  struct Uniform {
    glm::mat4 model2world;
    glm::mat4 world2view;
  };
  Uniform u;
  u.model2world = get_model2world();
  u.world2view = get_world2view();

  scoped::Buffer uniform_buf = ctxt.build_buf()
    .uniform()
    .streaming_with(u)
    .build();

  scoped::Buffer poses_buf = ctxt.build_buf()
    .vertex()
    .streaming_with_aligned(mesh.poses, sizeof(glm::vec4))
    .build();

  scoped::Buffer colors_buf = ctxt.build_buf()
    .storage()
    .streaming_with_aligned(colors, sizeof(glm::vec4))
    .build();

  scoped::Invocation lit_invoke = wireframe_task.build_graph_invoke()
    .vert_buf(poses_buf.view())
    .nvert(mesh.poses.size())
    .rsc(uniform_buf.view())
    .rsc(colors_buf.view())
    .build();

  rpib->invoke(lit_invoke);
  return *this;
}
Renderer& Renderer::draw_mesh_wireframe(const mesh::Mesh& mesh, const glm::vec3& color) {
  std::vector<glm::vec3> colors(mesh.poses.size(), color);
  return draw_mesh_wireframe(mesh, colors);
}
Renderer& Renderer::draw_mesh_wireframe(const mesh::Mesh& mesh) {
  return draw_mesh_wireframe(mesh, glm::vec3(1.0f, 1.0f, 0.0f));
}
Renderer& Renderer::draw_idxmesh_wireframe(const mesh::IndexedMesh& idxmesh, const std::vector<glm::vec3>& colors) {
  struct Uniform {
    glm::mat4 model2world;
    glm::mat4 world2view;
  };
  Uniform u;
  u.model2world = get_model2world();
  u.world2view = get_world2view();

  scoped::Buffer uniform_buf = ctxt.build_buf()
    .uniform()
    .streaming_with(u)
    .build();

  scoped::Buffer poses_buf = ctxt.build_buf()
    .vertex()
    .streaming_with_aligned(idxmesh.mesh.poses, sizeof(glm::vec4))
    .build();

  scoped::Buffer idxs_buf = ctxt.build_buf()
    .index()
    .streaming_with(idxmesh.idxs)
    .build();

  scoped::Buffer colors_buf = ctxt.build_buf()
    .storage()
    .streaming_with_aligned(colors, sizeof(glm::vec4))
    .build();

  scoped::Invocation lit_invoke = wireframe_task.build_graph_invoke()
    .vert_buf(poses_buf.view())
    .idx_buf(idxs_buf.view())
    .idx_ty(L_INDEX_TYPE_UINT32)
    .nidx(idxmesh.idxs.size() * 3)
    .rsc(uniform_buf.view())
    .rsc(colors_buf.view())
    .build();

  rpib->invoke(lit_invoke);
  return *this;
}
Renderer& Renderer::draw_idxmesh_wireframe(const mesh::IndexedMesh& idxmesh, const glm::vec3& color) {
  std::vector<glm::vec3> colors(idxmesh.mesh.poses.size(), color);
  return draw_idxmesh_wireframe(idxmesh, colors);
}
Renderer& Renderer::draw_idxmesh_wireframe(const mesh::IndexedMesh& idxmesh) {
  return draw_idxmesh_wireframe(idxmesh, glm::vec3(1.0f, 1.0f, 0.0f));
}
Renderer& Renderer::draw_point_cloud(const mesh::PointCloud& point_cloud, const std::vector<glm::vec3>& colors) {
  struct Uniform {
    glm::mat4 model2world;
    glm::mat4 world2view;
  };
  Uniform u;
  u.model2world = get_model2world();
  u.world2view = get_world2view();

  scoped::Buffer uniform_buf = ctxt.build_buf()
    .uniform()
    .streaming_with(u)
    .build();

  scoped::Buffer poses_buf = ctxt.build_buf()
    .vertex()
    .streaming_with_aligned(point_cloud.poses, sizeof(glm::vec4))
    .build();

  scoped::Buffer colors_buf = ctxt.build_buf()
    .storage()
    .streaming_with_aligned(colors, sizeof(glm::vec4))
    .build();

  scoped::Invocation invoke = point_cloud_task.build_graph_invoke()
    .vert_buf(poses_buf.view())
    .nvert(point_cloud.poses.size())
    .rsc(uniform_buf.view())
    .rsc(colors_buf.view())
    .build();

  rpib->invoke(invoke);
  return *this;
}
Renderer& Renderer::draw_point_cloud(const mesh::PointCloud& point_cloud, const glm::vec3& color) {
  std::vector<glm::vec3> colors(point_cloud.poses.size(), color);
  return draw_point_cloud(point_cloud, colors);
}
Renderer& Renderer::draw_point_cloud(const mesh::PointCloud& point_cloud) {
  return draw_point_cloud(point_cloud, glm::vec3(1.0f, 1.0f, 0.0f));
}

} // namespace scoped
} // namespace vk
} // namespace liong
