#include "log.hpp"
#include "vk.hpp"

using namespace liong;
using namespace vk;

namespace {

void log_cb(liong::log::LogLevel lv, const std::string& msg) {
  using liong::log::LogLevel;
  switch (lv) {
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

void guarded_main() {
  initialize();

  uint32_t ndev = 0;
  for (;;) {
    auto desc = desc_dev(ndev);
    if (desc.empty()) { break; }
    liong::log::info("device #", ndev, ": ", desc);
    ++ndev;
  }

  ContextConfig ctxt_cfg {};
  ctxt_cfg.label = "ctxt";
  ctxt_cfg.dev_idx = 0;
  scoped::Context ctxt(ctxt_cfg);

  ComputeTaskConfig comp_task_cfg {};
  comp_task_cfg.label = "comp_task";
  comp_task_cfg.entry_name = "main";
  comp_task_cfg.code = ext::load_code("../assets/arrange");
  comp_task_cfg.rsc_cfgs.push_back({ L_RESOURCE_TYPE_BUFFER, false });
  comp_task_cfg.workgrp_size = { 1, 1, 1 };
  scoped::Task task(ctxt, comp_task_cfg);

  BufferConfig buf_cfg {};
  buf_cfg.label = "buffer";
  buf_cfg.size = 16 * sizeof(float);
  buf_cfg.align = 1;
  buf_cfg.host_access = L_MEMORY_ACCESS_READ_WRITE;
  buf_cfg.dev_access = L_MEMORY_ACCESS_WRITE_ONLY;
  buf_cfg.is_const = false;
  scoped::Buffer buf(ctxt, buf_cfg);

  scoped::ResourcePool rsc_pool(ctxt, task);
  rsc_pool.bind(0, buf.view());

  std::vector<Command> secondary_cmds {
    cmd_dispatch(task, rsc_pool, { 4, 4, 4 }),
  };
  scoped::Transaction transact(ctxt, secondary_cmds);

  std::vector<Command> cmds {
    cmd_inline_transact(transact),
  };

  scoped::CommandDrain cmd_drain(ctxt);
  cmd_drain.submit(cmds);
  cmd_drain.wait();

  cmd_drain.submit(cmds);
  cmd_drain.wait();

  std::vector<float> dbuf;
  dbuf.resize(16);
  {
    scoped::MappedBuffer mapped(buf.view(), L_MEMORY_ACCESS_READ_ONLY);
    std::memcpy(dbuf.data(), (const void*)mapped, dbuf.size() * sizeof(float));
  }

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
