#include "gft/hal/builder.hpp"
#include "gft/log.hpp"
#include "gft/platform/macos.hpp"
#include "gft/platform/windows.hpp"
#include "gft/renderdoc.hpp"
#include "gft/vk/vk.hpp"
#include "gft/glslang.hpp"

using namespace liong;
using namespace liong::vk;
using namespace liong::fmt;

void dbg_enum_dev_descs(const InstanceRef& instance) {
  uint32_t ndev = 0;
  for (;;) {
    auto desc = instance->describe_device(ndev);
    if (desc.empty()) {
      break;
    }
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
    art.comp_spv.size() * sizeof(uint32_t)
  );
}
void dbg_dump_spv_art(
  const std::string& prefix,
  glslang::GraphicsSpirvArtifact art
) {
  liong::util::save_file(
    (prefix + ".vert.spv").c_str(),
    art.vert_spv.data(),
    art.vert_spv.size() * sizeof(uint32_t)
  );
  liong::util::save_file(
    (prefix + ".frag.spv").c_str(),
    art.frag_spv.data(),
    art.frag_spv.size() * sizeof(uint32_t)
  );
}

void guarded_main() {
  InstanceRef instance = VulkanInstance::create();

  dbg_enum_dev_descs(instance);

  std::string hlsl = R"(
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

#if defined(__MACH__) && defined(__APPLE__)
  macos::Window window = macos::create_window(1024, 768);
  ContextRef ctxt = instance->create_context( //
    ContextMetalConfig::build()               //
      .device_index(0)
      .metal_layer(window.metal_layer)
  );
#elif defined(_WIN32)
  windows::Window window = windows::create_window();
  ContextRef ctxt = instance->create_context( //
    WindowsContextConfig::build()             //
      .device_index(0)
      .hinst(window.hinst)
      .hwnd(window.hwnd)
  );
#else
  ContextRef ctxt = instance->create_context( //
    ContextConfig::build()                    //
      .device_index(0)
  );
#endif

  renderdoc::CaptureGuard capture;

  BufferRef ubo = ctxt->create_buffer( //
    BufferConfig::build()
      .label("ubo")
      .size(4 * sizeof(float))
      .uniform()
      .streaming()
  );

  {
    float data[4] { 0, 1, 0, 1 };
    ubo->copy_from(data);
  }

  BufferRef verts = ctxt->create_buffer( //
    BufferConfig::build()
      .label("verts")
      .size(3 * 3 * sizeof(float))
      .vertex()
      .streaming()
  );
  {
    float data[9] { 1, -1, 0, -1, -1, 0, -1, 1, 0 };
    verts->copy_from(data);
  }

  BufferRef idxs = ctxt->create_buffer( //
    BufferConfig::build()
      .label("idxs")
      .size(3 * 4 * sizeof(uint16_t))
      .index()
      .streaming()
  );
  {
    uint16_t data[3] { 0, 1, 2 };
    idxs->copy_from(data);
  }

  SwapchainRef swapchain = ctxt->create_swapchain( //
    SwapchainConfig::build()
      .label("swapchain")
      .image_count(3)
      .allowed_format(fmt::L_FORMAT_B8G8R8A8_UNORM)
      .allowed_format(fmt::L_FORMAT_R8G8B8A8_UNORM)
      .color_space(fmt::L_COLOR_SPACE_SRGB)
  );

  RenderPassRef pass = ctxt->create_render_pass( //
    RenderPassConfig::build()
      .label("pass")
      .width(swapchain->get_width())
      .height(swapchain->get_height())
      .clear_store_color_attachment(L_FORMAT_B8G8R8A8_UNORM, L_COLOR_SPACE_SRGB)
  );

  auto art = glslang::compile_graph_hlsl(hlsl, "vert", hlsl, "frag");

  TaskRef task = pass->create_graphics_task( //
    GraphicsTaskConfig::build()
      .label("graph_task")
      .vertex_shader(art.vert_spv, "vert")
      .fragment_shader(art.frag_spv, "frag")
      .uniform_buffer()
  );

  for (;;) {
    ImageRef out_img = swapchain->get_current_image();

    InvocationRef draw_call = task->create_graphics_invocation( //
      GraphicsInvocationConfig::build()
        .label("draw_call")
        .vertex_buffer(verts->view())
        .per_u32_index(idxs->view(), 3)
        .resource(ubo->view())
    );

    InvocationRef main_pass =
      pass->create_render_pass_invocation(RenderPassInvocationConfig::build()
                                            .label("main_pass")
                                            .attachment(out_img->view())
                                            .invocation(draw_call));

    main_pass->create_transact(TransactionConfig::build())->wait();

    swapchain->create_present_invocation(PresentInvocationConfig::build())
      ->create_transact(TransactionConfig::build())
      ->wait();
  }
}

int main(int argc, char** argv) {
  try {
    renderdoc::initialize();
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
