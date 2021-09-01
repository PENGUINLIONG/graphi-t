#include "log.hpp"
#include "vk.hpp"
#include "glslang.hpp"

using namespace liong;
using namespace vk;

namespace {

void log_cb(liong::log::LogLevel lv, const std::string& msg) {
  using liong::log::LogLevel;
  switch (lv) {
  case LogLevel::L_LOG_LEVEL_DEBUG:
    printf("[\x1b[90mDEBUG\x1B[0m] %s\n", msg.c_str());
    break;
  case LogLevel::L_LOG_LEVEL_INFO:
    printf("[\x1B[32mINFO\x1B[0m] %s\n", msg.c_str());
    break;
  case LogLevel::L_LOG_LEVEL_WARNING:
    printf("[\x1B[33mWARN\x1B[0m] %s\n", msg.c_str());
    break;
  case LogLevel::L_LOG_LEVEL_ERROR:
    printf("[\x1B[31mERROR\x1B[0m] %s\n", msg.c_str());
    break;
  }
}

} // namespace



void copy_buf2host(
  const BufferView& src,
  void* dst,
  size_t size
) {
  if (size == 0) {
    liong::log::warn("zero-sized copy is ignored");
    return;
  }
  liong::assert(src.size >= size, "src buffer size is too small");
  scoped::MappedBuffer mapped(src, L_MEMORY_ACCESS_READ_ONLY);
  std::memcpy(dst, (const void*)mapped, size);
}
void copy_host2buf(
  const void* src,
  const BufferView& dst,
  size_t size
) {
  if (size == 0) {
    liong::log::warn("zero-sized copy is ignored");
    return;
  }
  liong::assert(dst.size >= size, "dst buffser size is too small");
  scoped::MappedBuffer mapped(dst, L_MEMORY_ACCESS_WRITE_ONLY);
  std::memcpy((void*)mapped, mapped, size);
}



void dbg_enum_dev_descs() {
  uint32_t ndev = 0;
  for (;;) {
    auto desc = desc_dev(ndev);
    if (desc.empty()) { break; }
    liong::log::info("device #", ndev, ": ", desc);
    ++ndev;
  }
}
void dbg_dump_spv_art(
  const std::string& prefix,
  glslang::ComputeSpirvArtifact art
) {
  liong::util::save_file(
    (prefix + ".comp.spv").c_str(),
    art.comp_spv.data(),
    art.comp_spv.size() * sizeof(uint32_t));
}
void dbg_dump_spv_art(
  const std::string& prefix,
  glslang::GraphicsSpirvArtifact art
) {
  liong::util::save_file(
    (prefix + ".vert.spv").c_str(),
    art.vert_spv.data(),
    art.vert_spv.size() * sizeof(uint32_t));
  liong::util::save_file(
    (prefix + ".frag.spv").c_str(),
    art.frag_spv.data(),
    art.frag_spv.size() * sizeof(uint32_t));
}







void guarded_main() {
  vk::initialize();
  glslang::initialize();

  dbg_enum_dev_descs();

  std::string glsl = R"(
    #version 450 core
    layout(local_size_x_id=0, local_size_y_id=1, local_size_z_id=2) in;
    layout(binding=0) writeonly buffer Buffer {
      float data[];
    };

    void comp() {
      uvec3 size = gl_NumWorkGroups;
      uvec3 id = gl_WorkGroupID;
      uint idx = id.x + size.x * (id.y + size.y * id.z);
      data[idx] = float(idx);
    }
  )";
  glslang::ComputeSpirvArtifact art =
    glslang::compile_comp(glsl.c_str(), "comp");
  dbg_dump_spv_art("out", art);

  scoped::Context ctxt("ctxt", 0);

  scoped::Task task = ctxt.create_comp_task("comp_task", "main", art.comp_spv,
    { 1, 1, 1 }, { L_RESOURCE_TYPE_STORAGE_BUFFER });

  scoped::Buffer buf = ctxt.create_storage_buf("buf", 16 * sizeof(float));

  scoped::ResourcePool rsc_pool = task.create_rsc_pool();
  rsc_pool.bind(0, buf.view());

  scoped::Transaction transact = ctxt.create_transact("transact", {
    cmd_dispatch(task, rsc_pool, { 4, 4, 4 }),
  });

  scoped::CommandDrain cmd_drain(ctxt);
  cmd_drain.submit({
    cmd_inline_transact(transact),
  });
  cmd_drain.wait();

  cmd_drain.submit({
    cmd_inline_transact(transact),
  });
  cmd_drain.wait();

  std::vector<float> dbuf;
  dbuf.resize(16);
  copy_buf2host(buf.view(), dbuf.data(), dbuf.size() * sizeof(float));

  for (auto i = 0; i < dbuf.size(); ++i) {
    liong::log::info("dbuf[", i, "] = ", dbuf[i]);
  }
}

void guarded_main2() {
  vk::initialize();
  glslang::initialize();

  dbg_enum_dev_descs();

  std::string vert_glsl = R"(
    void vert(
      in float4 InPosition: ATTRIBUTE0,
      out float4 OutColor: TEXCOORD0,
      out float4 OutPosition: SV_POSITION
    ) {
      OutColor = float4(1.0f, 1.0f, 0.0f, 1.0f);
      OutPosition = float4(InPosition.xy, 0.0f, 1.0f);
    }
    float4 ColorMultiplier;

    half4 frag(
      in float4 InColor: TEXCOORD
    ) : SV_TARGET {
      return half4((InColor * ColorMultiplier));
    }
  )";
  glslang::GraphicsSpirvArtifact art =
    glslang::compile_graph_hlsl(vert_glsl, "vert", vert_glsl, "frag");
  dbg_dump_spv_art("out", art);

  liong::assert(art.ubo_size == 4 * sizeof(float),
    "unexpected ubo size; should be 16, but is ", art.ubo_size);

  scoped::Context ctxt("ctxt", 0);

  std::vector<ResourceType> rsc_tys {
    L_RESOURCE_TYPE_UNIFORM_BUFFER,
  };

  scoped::Task task = ctxt.create_graph_task("graph_task", "main", art.vert_spv,
    "main", art.frag_spv, L_TOPOLOGY_TRIANGLE, rsc_tys);

  scoped::Buffer ubo = ctxt.create_uniform_buf("ubo", 4 * sizeof(float));
  {
    float data[4] {
      0, 1, 0, 1
    };
    scoped::MappedBuffer mapped = ubo.map(L_MEMORY_ACCESS_WRITE_ONLY);
    float* ubo_data = (float*)mapped;
    std::memcpy(ubo_data, data, sizeof(data));
  }

  scoped::ResourcePool rsc_pool = task.create_rsc_pool();
  rsc_pool.bind(0, ubo.view());

  scoped::Buffer verts = ctxt.create_vert_buf("verts", 3 * 4 * sizeof(float));
  {
    float data[12] {
       1, -1, 0, 1,
      -1, -1, 0, 1,
      -1,  1, 0, 1,
    };
    scoped::MappedBuffer mapped = verts.map(L_MEMORY_ACCESS_WRITE_ONLY);
    float* verts_data = (float*)mapped;
    std::memcpy(verts_data, data, sizeof(data));
  }

  scoped::Buffer idxs = ctxt.create_idx_buf("idxs", 3 * 4 * sizeof(uint16_t));
  {
    uint16_t data[3] {
      0, 1, 2
    };
    scoped::MappedBuffer mapped = idxs.map(L_MEMORY_ACCESS_WRITE_ONLY);
    uint16_t* idxs_data = (uint16_t*)mapped;
    std::memcpy(idxs_data, data, sizeof(data));
  }


  constexpr uint32_t FRAMEBUF_NCOL = 4;
  constexpr uint32_t FRAMEBUF_NROW = 4;

  scoped::Image out_img = ctxt.create_attm_img("attm", 4, 4,
    L_FORMAT_R32G32B32A32_SFLOAT);

  scoped::Framebuffer framebuf = task.create_framebuf(out_img);

  scoped::Buffer out_buf = ctxt.create_staging_buf("out_buf",
    FRAMEBUF_NCOL * FRAMEBUF_NROW * 4 * sizeof(float));

  scoped::Timestamp tic = ctxt.create_timestamp();
  scoped::Timestamp toc = ctxt.create_timestamp();

  std::vector<Command> cmds {
    cmd_write_timestamp(tic, L_SUBMIT_TYPE_GRAPHICS),
    cmd_draw_indexed(task, rsc_pool, idxs.view(), verts.view(), 3, 1, framebuf),
    cmd_write_timestamp(toc, L_SUBMIT_TYPE_GRAPHICS),
    cmd_copy_img2buf(out_img.view(), out_buf.view()),
  };

  scoped::CommandDrain cmd_drain = ctxt.create_cmd_drain();
  cmd_drain.submit(cmds);
  cmd_drain.wait();

  liong::log::warn("drawing took ", toc.get_result_us() - tic.get_result_us(), "us");

  {
    scoped::MappedBuffer mapped = out_buf.map(L_MEMORY_ACCESS_READ_ONLY);
    const float* out_data = (const float*)mapped;
    std::stringstream ss;

    liong::util::save_bmp(out_data, FRAMEBUF_NCOL, FRAMEBUF_NROW, "out_img.bmp");
  }
}


int main(int argc, char** argv) {
  liong::log::set_log_callback(log_cb);
  try {
    guarded_main();
    guarded_main2();
  } catch (const std::exception& e) {
    liong::log::error("application threw an exception");
    liong::log::error(e.what());
    liong::log::error("application cannot continue");
  } catch (...) {
    liong::log::error("application threw an illiterate exception");
  }

  return 0;
}
