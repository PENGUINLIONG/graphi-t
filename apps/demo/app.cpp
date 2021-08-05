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
  Context ctxt = create_ctxt(ctxt_cfg);

  ComputeTaskConfig comp_task_cfg {};
  comp_task_cfg.label = "comp_task";
  comp_task_cfg.entry_name = "arrange";
  comp_task_cfg.code = ext::load_code("../assets/arrange");
  comp_task_cfg.rsc_cfgs.push_back({ L_RESOURCE_TYPE_BUFFER, false });
  comp_task_cfg.workgrp_size = { 1, 1, 1 };
  Task task = create_comp_task(ctxt, comp_task_cfg);

  BufferConfig buf_cfg {};
  buf_cfg.label = "buffer";
  buf_cfg.size = 16 * sizeof(float);
  buf_cfg.align = 1;
  buf_cfg.host_access = L_MEMORY_ACCESS_READ_WRITE;
  buf_cfg.dev_access = L_MEMORY_ACCESS_WRITE_ONLY;
  buf_cfg.is_const = false;
  Buffer buf = create_buf(ctxt, buf_cfg);

  BufferView buf_view { &buf, 0, buf.buf_cfg.size };

  ResourcePool rsc_pool = create_rsc_pool(ctxt, task);
  bind_pool_rsc(rsc_pool, 0, buf_view);

  Command secondary_cmds[] = {
    cmd_dispatch(task, rsc_pool, { 4, 4, 4 }),
  };

  Transaction transact = create_transact(ctxt, secondary_cmds,
    sizeof(secondary_cmds) / sizeof(Command));

  Command cmds[] = {
    cmd_inline_transact(transact),
  };

  CommandDrain cmd_drain = create_cmd_drain(ctxt);
  submit_cmds(cmd_drain, cmds, sizeof(cmds) / sizeof(Command));
  wait_cmd_drain(cmd_drain);

  submit_cmds(cmd_drain, cmds, sizeof(cmds) / sizeof(Command));
  wait_cmd_drain(cmd_drain);

  void* mapped;
  std::vector<float> dbuf;
  dbuf.resize(16);
  map_mem(buf_view, mapped, L_MEMORY_ACCESS_READ_ONLY);

  std::memcpy(dbuf.data(), mapped, dbuf.size() * sizeof(float));

  unmap_mem(buf_view, mapped);

  for (auto i = 0; i < dbuf.size(); ++i) {
    liong::log::info("dbuf[", i, "] = ", dbuf[i]);
  }

  destroy_cmd_drain(cmd_drain);
  destroy_transact(transact);
  destroy_rsc_pool(rsc_pool);
  destroy_buf(buf);
  destroy_task(task);
  destroy_ctxt(ctxt);
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
