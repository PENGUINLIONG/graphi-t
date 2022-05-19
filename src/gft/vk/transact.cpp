#include "gft/vk.hpp"
#include "gft/log.hpp"

namespace liong {
namespace vk {

void destroy_transact(Transaction& transact) {
  const Context& ctxt = *transact.ctxt;
  for (auto& submit_detail : transact.submit_details) {
    if (submit_detail.signal_sema != VK_NULL_HANDLE) {
      vkDestroySemaphore(ctxt.dev, submit_detail.signal_sema, nullptr);
    }
    if (submit_detail.cmd_pool != VK_NULL_HANDLE) {
      vkDestroyCommandPool(ctxt.dev, submit_detail.cmd_pool, nullptr);
    }
  }

  for (const auto& fence : transact.fences) {
    vkDestroyFence(ctxt.dev, fence, nullptr);
  }

  log::debug("destroyed transaction");
  transact = {};
}
bool is_transact_done(const Transaction& transact) {
  const Context& ctxt = *transact.ctxt;
  for (const auto& fence : transact.fences) {
    VkResult err = vkGetFenceStatus(ctxt.dev, fence);
    if (err == VK_NOT_READY) {
      return false;
    } else {
      VK_ASSERT << err;
    }
  }
  return true;
}
void wait_transact(const Transaction& transact) {
  const uint32_t SPIN_INTERVAL = 3000;

  const Context& ctxt = *transact.ctxt;

  util::Timer wait_timer {};
  wait_timer.tic();
  for (VkResult err;;) {
    err = vkWaitForFences(ctxt.dev, transact.fences.size(),
      transact.fences.data(), VK_TRUE, SPIN_INTERVAL);
    if (err == VK_TIMEOUT) {
      // log::warn("timeout after 3000ns");
    } else {
      VK_ASSERT << err;
      break;
    }
  }
  wait_timer.toc();

  log::debug("command drain returned after ", wait_timer.us(), "us since the "
    "wait started (spin interval = ", SPIN_INTERVAL / 1000.0, "us");
}

} // namespace vk
} // namespace liong
