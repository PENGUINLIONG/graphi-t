#include "gft/log.hpp"
#include "gft/vk.hpp"
#include "gft/glslang.hpp"

using namespace liong;
using namespace vk;
using namespace fmt;

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
  scoped::MappedBuffer mapped(src, L_MEMORY_ACCESS_READ_BIT);
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
  scoped::MappedBuffer mapped(dst, L_MEMORY_ACCESS_WRITE_BIT);
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
  scoped::GcScope scope;

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

  scoped::Context ctxt = create_ctxt(0, "ctxt");

  scoped::Task task = ctxt.build_comp_task("comp_task")
    .comp(art.comp_spv)
    .rsc(L_RESOURCE_TYPE_STORAGE_BUFFER)
    .build();

  scoped::Buffer buf = ctxt.build_buf("buf")
    .size(16 * sizeof(float))
    .storage()
    .streaming()
    .read_back()
    .build();

  scoped::Invocation invoke = task.build_comp_invoke()
    .rsc(buf.view())
    .workgrp_count(4, 4, 4)
    .build();
  invoke.bake();

  invoke.submit().wait();
  invoke.submit().wait();

  std::vector<float> dbuf;
  dbuf.resize(16);
  copy_buf2host(buf.view(), dbuf.data(), dbuf.size() * sizeof(float));

  for (auto i = 0; i < dbuf.size(); ++i) {
    liong::log::info("dbuf[", i, "] = ", dbuf[i]);
  }

  std::vector<float> pattern;
  {
    pattern.resize(7 * 7 * 4);
    for (int i = 0; i < 7; ++i) {
      for (int j = i; j < 7; ++j) {
        pattern[(i * 7 + j) * 4 + 0] = 1.0f;
        pattern[(i * 7 + j) * 4 + 1] = 0.0f;
        pattern[(i * 7 + j) * 4 + 2] = 1.0f;
        pattern[(i * 7 + j) * 4 + 3] = 1.0f;
      }
    }
  }

}

void guarded_main2() {
  scoped::GcScope scope;

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

  scoped::Context ctxt = create_ctxt(0, "ctxt");

  constexpr uint32_t FRAMEBUF_WIDTH = 4;
  constexpr uint32_t FRAMEBUF_HEIGHT = 4;

  scoped::Buffer ubo = ctxt.build_buf("ubo")
    .size(4 * sizeof(float))
    .uniform()
    .streaming()
    .build();
  {
    float data[4] {
      0, 1, 0, 1
    };
    ubo.map_write().write(data);
  }

  scoped::Buffer verts = ctxt.build_buf("verts")
    .size(3 * 4 * sizeof(float))
    .vertex()
    .streaming()
    .build();
  {
    float data[12] {
       1, -1, 0, 1,
      -1, -1, 0, 1,
      -1,  1, 0, 1,
    };
    verts.map_write().write(data);
  }

  scoped::Buffer idxs = ctxt.build_buf("idxs")
    .size(3 * 4 * sizeof(uint16_t))
    .index()
    .streaming()
    .build();
  {
    uint16_t data[3] {
      0, 1, 2
    };
    idxs.map_write().write(data);
  }

  scoped::DepthImage zbuf = ctxt.build_depth_img("zbuf")
    .width(4)
    .height(4)
    .fmt(L_DEPTH_FORMAT_D16_UNORM)
    .attachment()
    .tile_memory()
    .build();
  scoped::Image out_img = ctxt.build_img("attm")
    .width(4)
    .height(4)
    .fmt(L_FORMAT_R32G32B32A32_SFLOAT)
    .attachment()
    .build();



  scoped::RenderPass pass = ctxt.build_pass("pass")
    .width(FRAMEBUF_WIDTH)
    .height(FRAMEBUF_HEIGHT)
    .clear_store_attm(L_FORMAT_R32G32B32A32_SFLOAT)
    .clear_store_attm(L_DEPTH_FORMAT_D16_UNORM)
    .build();

  scoped::Task task = pass.build_graph_task("graph_task")
    .vert(art.vert_spv)
    .frag(art.frag_spv)
    .per_vert_input(L_FORMAT_R32G32B32A32_SFLOAT)
    .rsc(L_RESOURCE_TYPE_UNIFORM_BUFFER)
    .build();

  scoped::Invocation draw_call = task.build_graph_invoke("draw_call")
    .vert_buf(verts.view())
    .idx_buf(idxs.view())
    .rsc(ubo.view())
    .nidx(3)
    .build();

  scoped::Invocation main_pass = pass.build_pass_invoke("main_pass")
    .attm(out_img.view())
    .attm(zbuf.view())
    .invoke(draw_call)
    .is_timed()
    .build();

  scoped::Buffer out_buf = ctxt.build_buf("out_buf")
    .size(FRAMEBUF_WIDTH * FRAMEBUF_HEIGHT * 4 * sizeof(float))
    .read_back()
    .build();

  scoped::Invocation write_back = ctxt.build_trans_invoke("write_back")
    .src(out_img.view())
    .dst(out_buf.view())
    .build();

  scoped::Invocation proc = ctxt.build_composite_invoke("proc")
    .invoke(main_pass)
    .invoke(write_back)
    .build();

  proc.submit().wait();

  liong::log::warn("drawing took ", main_pass.get_time_us(), "us");

  {
    scoped::MappedBuffer mapped = out_buf.map(L_MEMORY_ACCESS_READ_BIT);
    const float* out_data = (const float*)mapped;
    liong::util::save_bmp(out_data, FRAMEBUF_WIDTH, FRAMEBUF_HEIGHT, "out_img.bmp");
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
