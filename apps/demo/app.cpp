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
    printf("[DEBUG] %s\n", msg.c_str());
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

  std::vector<ResourceConfig> rsc_cfgs {
    { L_RESOURCE_TYPE_BUFFER, false },
  };
  DispatchSize local_size { 1, 1, 1 };
  scoped::Task task = ctxt.create_comp_task("comp_task",
    "main",
    art.comp_spv,
    rsc_cfgs,
    local_size);

  scoped::Buffer buf = ctxt.create_storage_buf("buf",
    L_MEMORY_ACCESS_READ_WRITE,
    L_MEMORY_ACCESS_WRITE_ONLY,
    16 * sizeof(float));

  scoped::ResourcePool rsc_pool = task.create_rsc_pool();
  rsc_pool.bind(0, buf.view());

  scoped::Transaction transact = ctxt.create_transact({
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

int main(int argc, char** argv) {
  liong::log::set_log_callback(log_cb);
  try {
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
