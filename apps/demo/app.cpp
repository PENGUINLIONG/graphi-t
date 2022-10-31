#include "gft/log.hpp"
#include "gft/vk.hpp"
#include "gft/glslang.hpp"
#include "gft/renderdoc.hpp"

#if defined(_WIN32)
#include "gft/platform/windows.hpp"
#endif // defined(_WIN32)

#if defined(__MACH__) && defined(__APPLE__)
#include "gft/platform/macos.hpp"
#endif // defined(__MACH__) && defined(__APPLE__)

using namespace liong;
using namespace vk;
using namespace fmt;

void copy_buf2host(
  const BufferView& src,
  void* dst,
  size_t size
) {
  if (size == 0) {
    L_WARN("zero-sized copy is ignored");
    return;
  }
  L_ASSERT(src.size >= size, "src buffer size is too small");
  scoped::MappedBuffer mapped(src, L_MEMORY_ACCESS_READ_BIT);
  std::memcpy(dst, (const void*)mapped, size);
}
void copy_host2buf(
  const void* src,
  const BufferView& dst,
  size_t size
) {
  if (size == 0) {
    L_WARN("zero-sized copy is ignored");
    return;
  }
  L_ASSERT(dst.size >= size, "dst buffser size is too small");
  scoped::MappedBuffer mapped(dst, L_MEMORY_ACCESS_WRITE_BIT);
  std::memcpy((void*)mapped, mapped, size);
}



void dbg_enum_dev_descs() {
  uint32_t ndev = 0;
  for (;;) {
    auto desc = desc_dev(ndev);
    if (desc.empty()) { break; }
    L_INFO("device #", ndev, ": ", desc);
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

  ContextConfig ctxt_cfg { "ctxt", 0 };
  scoped::Context ctxt = scoped::Context::own_by_gc_frame(create_ctxt(ctxt_cfg));

  auto x = ctxt.inner->acquire_query_pool();
  x.release();

  return;
#if 0
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

  L_ASSERT(art.ubo_size == 4 * sizeof(float),
    "unexpected ubo size; should be 16, but is ", art.ubo_size);

#if defined(__MACH__) && defined(__APPLE__)
  macos::Window window = macos::create_window(1024, 768);
  ContextMetalConfig ctxt_metal_cfg { "ctxt", 0, window.metal_layer };
  scoped::Context ctxt = scoped::Context::own_by_gc_frame(create_ctxt_metal(ctxt_metal_cfg));
#elif defined(_WIN32)
  windows::Window window = windows::create_window();
  ContextWindowsConfig ctxt_cfg { "ctxt", 0, window.hinst, window.hwnd };
  scoped::Context ctxt = scoped::Context::own_by_gc_frame(create_ctxt_windows(ctxt_cfg));
#else
  ContextConfig ctxt_cfg { "ctxt", 0 };
  scoped::Context ctxt = scoped::Context::own_by_gc_frame(create_ctxt(ctxt_cfg));
#endif

  renderdoc::CaptureGuard capture;

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
    .size(3 * 3 * sizeof(float))
    .vertex()
    .streaming()
    .build();
  {
    float data[9] {
       1, -1, 0,
      -1, -1, 0,
      -1,  1, 0,
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



  scoped::Swapchain swapchain = ctxt.build_swapchain("swapchain")
    .build();

  scoped::RenderPass pass = ctxt.build_pass("pass")
    .width(swapchain.width())
    .height(swapchain.height())
    .clear_store_attm(L_FORMAT_B8G8R8A8_UNORM)
    .build();

  scoped::Task task = pass.build_graph_task("graph_task")
    .vert(art.vert_spv)
    .frag(art.frag_spv)
    .rsc(L_RESOURCE_TYPE_UNIFORM_BUFFER)
    .build();

  for (;;) {
    scoped::Image out_img = swapchain.get_img();

    scoped::Invocation draw_call = task.build_graph_invoke("draw_call")
      .vert_buf(verts.view())
      .idx_buf(idxs.view())
      .rsc(ubo.view())
      .nidx(3)
      .build();

    scoped::Invocation main_pass = pass.build_pass_invoke("main_pass")
      .attm(out_img.view())
      .invoke(draw_call)
      .build();

    main_pass.submit().wait();

    swapchain.create_present_invoke().submit().wait();
  }
#endif

}


int main(int argc, char** argv) {
  try {
    renderdoc::initialize();
    vk::initialize();
    glslang::initialize();

    guarded_main();
  } catch (const std::exception& e) {
    L_ERROR("application threw an exception");
    L_ERROR(e.what());
    L_ERROR("application cannot continue");
  } catch (...) {
    L_ERROR("application threw an illiterate exception");
  }

  return 0;
}
