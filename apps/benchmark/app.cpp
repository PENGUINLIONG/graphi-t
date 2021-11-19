#include "log.hpp"
#include "vk.hpp"
#include "glslang.hpp"
#include "json.hpp"
#include "args.hpp"

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

struct AppConfig {
  std::string frag_src;
  std::string frag_entry_name;
  uint32_t frag_nin;

  uint32_t width;
  uint32_t height;

  uint32_t nrepeat = 10;
  bool dump_framebuf = true;
} CFG;

void initialize(int argc, const char** argv) {
  using namespace liong::args;

  std::string cfg_json_path = "./cfg.json";
  init_arg_parse("Benchmark", "Measure shader execution time on GPU.");
  args::reg_arg<StringParser>("", "--config", cfg_json_path,
    "Path to configuration json file. If the specified file doesn't exists, a "
    "template json will be emitted. (Default=./cfg.json)");
  args::reg_arg<UintParser>("-n", "--nrepeat", CFG.nrepeat,
    "Number of times to repeat execution and timing, the final output will be "
    "the average of all timing iterations (Default=10)");
  args::reg_arg<SwitchParser>("", "--dump-framebuf", CFG.dump_framebuf,
    "Dump framebuffer content to image file.");
  parse_args(argc, argv);

  liong::assert(!cfg_json_path.empty(), "configuration json must be specified");

  std::string cfg_json_txt;
  try {
    cfg_json_txt = liong::util::load_text(cfg_json_path.c_str());
  } catch (...) {
    json::JsonValue cfg_json = json::JsonObject {
      { "FragmentShader", json::JsonObject {
          { "HlslSource", json::JsonValue(R"(
float4 DummyFactor;
half4 frag(in float4 InColor: TEXCOORD0) : SV_TARGET {
  return half4(1, 0, 1, 1) + DummyFactor;
}
            )")
          },
          { "EntryPointName", json::JsonValue("frag") },
          { "InputCount", json::JsonValue(1), },
        },
      },
      { "Framebuffer", json::JsonObject {
          { "Width", json::JsonValue(256) },
          { "Height", json::JsonValue(256) },
        },
      },
    };
    cfg_json_txt = json::print(cfg_json);
    liong::util::save_text(cfg_json_path.c_str(), cfg_json_txt);
  }
  liong::json::JsonValue cfg_json = liong::json::parse(cfg_json_txt);
  liong::json::JsonValue frag_cfg_json = cfg_json["FragmentShader"];
  CFG.frag_src = (std::string)frag_cfg_json["HlslSource"];
  CFG.frag_entry_name = (std::string)frag_cfg_json["EntryPointName"];
  CFG.frag_nin = frag_cfg_json["InputCount"];
  liong::json::JsonValue framebuf_cfg_json = cfg_json["Framebuffer"];
  CFG.width = framebuf_cfg_json["Width"];
  CFG.height = framebuf_cfg_json["Height"];

  vk::initialize();
  glslang::initialize();
}




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
  dbg_enum_dev_descs();

  std::string vert_hlsl = liong::util::format(R"(
    void vert(
      in float4 InPosition: ATTRIBUTE0,)", []{
        std::stringstream ss;
        for (auto i = 0; i < CFG.frag_nin; ++i) {
          ss << "out float4 " << "TexCoord" << i << ": TEXCOORD" << i << ",";
        }
        return ss.str();
      }(), R"(
      out float4 OutPosition: SV_POSITION
    ) {
      OutPosition = float4(InPosition.xy, 0.0f, 1.0f);)", []{
        std::stringstream ss;
        for (auto i = 0; i < CFG.frag_nin; ++i) {
          ss << "TexCoord" << i << " = OutPosition;";
        }
        return ss.str();
      }(), R"(
    }
  )");
  glslang::GraphicsSpirvArtifact art = glslang::compile_graph_hlsl(vert_hlsl,
    "vert", CFG.frag_src, CFG.frag_entry_name);
  dbg_dump_spv_art("out", art);

  scoped::Context ctxt("ctxt", 0);
  // DO NOT create command drain after command vector declaration. That leads to
  //segfault. (??? WTF)
  scoped::CommandDrain cmd_drain = ctxt.create_cmd_drain();

  scoped::Image out_img = ctxt.create_attm_img("attm", CFG.width, CFG.height,
    L_FORMAT_R32G32B32A32_SFLOAT);
  scoped::DepthImage zbuf = ctxt.create_depth_img("zbuf", CFG.width, CFG.height,
    L_DEPTH_FORMAT_D16_S0);

  std::vector<AttachmentConfig> attm_cfgs;
  {
    AttachmentConfig attm_cfg {};
    attm_cfg.attm_ty = L_ATTACHMENT_TYPE_COLOR;
    attm_cfg.attm_access =
            (AttachmentAccess)(L_ATTACHMENT_ACCESS_CLEAR | L_ATTACHMENT_ACCESS_STORE);
    attm_cfg.color_img = &(const Image&)out_img;
    attm_cfgs.emplace_back(attm_cfg);
  }
  {
    AttachmentConfig attm_cfg {};
    attm_cfg.attm_ty = L_ATTACHMENT_TYPE_DEPTH;
    attm_cfg.attm_access =
            (AttachmentAccess)(L_ATTACHMENT_ACCESS_LOAD | L_ATTACHMENT_ACCESS_STORE);
    attm_cfg.depth_img = &(const DepthImage&)zbuf;
    attm_cfgs.emplace_back(attm_cfg);
  }

  scoped::RenderPass pass = ctxt.create_pass("pass", attm_cfgs, CFG.width, CFG.height);

  std::vector<ResourceType> rsc_tys {
    L_RESOURCE_TYPE_UNIFORM_BUFFER,
  };
  scoped::Task task = pass.create_graph_task("graph_task", "main", art.vert_spv,
    "main", art.frag_spv, { VertexInput { L_FORMAT_R32G32B32A32_SFLOAT, L_VERTEX_INPUT_RATE_VERTEX }}, L_TOPOLOGY_TRIANGLE, rsc_tys);

  // 16 is just a random non-zero number.
  scoped::Buffer ubo = ctxt.create_uniform_buf("ubo",
    std::max(art.ubo_size, (size_t)16));

  scoped::ResourcePool rsc_pool = task.create_rsc_pool();
  rsc_pool.bind(0, ubo.view());

  scoped::Buffer verts = ctxt.create_vert_buf("verts", 16 * sizeof(float));
  {
    float data[16] {
       1, -1, 0, 1,
      -1, -1, 0, 1,
      -1,  1, 0, 1,
       1,  1, 0, 1,
    };
    scoped::MappedBuffer mapped = verts.map(L_MEMORY_ACCESS_WRITE_ONLY);
    float* verts_data = (float*)mapped;
    std::memcpy(verts_data, data, sizeof(data));
  }

  scoped::Buffer idxs = ctxt.create_idx_buf("idxs", 6 * sizeof(uint16_t));
  {
    uint16_t data[6] {
      0, 1, 2,
      0, 2, 3,
    };
    scoped::MappedBuffer mapped = idxs.map(L_MEMORY_ACCESS_WRITE_ONLY);
    uint16_t* idxs_data = (uint16_t*)mapped;
    std::memcpy(idxs_data, data, sizeof(data));
  }


  auto bench = [&](bool dump_framebuf) {
    scoped::Buffer out_buf = ctxt.create_staging_buf("out_buf",
      CFG.width * CFG.height * 4 * sizeof(float));

    scoped::Timestamp tic = ctxt.create_timestamp();
    scoped::Timestamp toc = ctxt.create_timestamp();

    std::vector<Command> cmds {
      cmd_set_submit_ty(L_SUBMIT_TYPE_GRAPHICS),
      cmd_img_barrier(out_img,
                      L_IMAGE_USAGE_NONE,
                      L_IMAGE_USAGE_ATTACHMENT_BIT,
                      L_MEMORY_ACCESS_NONE,
                      L_MEMORY_ACCESS_WRITE_ONLY),
      cmd_depth_img_barrier(zbuf,
                            L_DEPTH_IMAGE_USAGE_NONE,
                            L_DEPTH_IMAGE_USAGE_ATTACHMENT_BIT,
                            L_MEMORY_ACCESS_NONE,
                            L_MEMORY_ACCESS_WRITE_ONLY),
      cmd_write_timestamp(tic),
      cmd_begin_pass(pass, true),
      cmd_draw_indexed(task, rsc_pool, idxs.view(), verts.view(), 6, 1),
      cmd_end_pass(pass),
      cmd_write_timestamp(toc),
      cmd_copy_img2buf(out_img.view(), out_buf.view()),
    };

    cmd_drain.submit(cmds);
    cmd_drain.wait();

    double dt = toc.get_result_us() - tic.get_result_us();
    liong::log::warn("drawing took ", dt, "us");

    // Dump output image.
    {
      scoped::MappedBuffer mapped = out_buf.map(L_MEMORY_ACCESS_READ_ONLY);
      const float* out_data = (const float*)mapped;
      liong::util::save_bmp(out_data, CFG.width, CFG.height, "out_img.bmp");
    }

    return dt;
  };


  bench(CFG.dump_framebuf);
  bench(false);

  double mean_dt = 0.0f;
  for (auto i = 0; i < CFG.nrepeat; ++i) {
    mean_dt += bench(false);
  }
  mean_dt /= CFG.nrepeat;
  liong::log::warn("drawing took ", mean_dt, "us (", CFG.nrepeat,
    " times average)");
}

int main(int argc, const char** argv) {
  liong::log::set_log_callback(log_cb);
  try {
    initialize(argc, argv);
    guarded_main();
  } catch (const std::exception& e) {
    liong::log::error("application threw an exception");
    liong::log::error(e.what());
    liong::log::error("application cannot continue");
  } catch (...) {
    liong::log::error("application threw an illiterate exception");
  }

  return 0;
}
