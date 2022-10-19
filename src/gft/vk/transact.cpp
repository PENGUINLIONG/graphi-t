#include "gft/vk.hpp"
#include "gft/log.hpp"

namespace liong {
namespace vk {

void destroy_transact(Transaction& transact) {
  const Context& ctxt = *transact.ctxt;
  log::debug("destroyed transaction");
  transact = {};
}
bool is_transact_done(const Transaction& transact) {
  const Context& ctxt = *transact.ctxt;
  for (const auto& fence : transact.fences) {
    VkResult err = vkGetFenceStatus(ctxt.dev->dev, fence->fence);
    if (err == VK_NOT_READY) {
      return false;
    } else {
      VK_ASSERT << err;
    }
  }
  return true;
}
void wait_transact(const Transaction& transact) {
  const Context& ctxt = *transact.ctxt;

  std::vector<VkFence> fences(transact.fences.size());
  for (size_t i = 0; i < fences.size(); ++i) {
    fences.at(i) = transact.fences.at(i)->fence;
  }

  util::Timer wait_timer {};
  wait_timer.tic();
  for (VkResult err;;) {
    err = vkWaitForFences(ctxt.dev->dev, transact.fences.size(),
      fences.data(), VK_TRUE, SPIN_INTERVAL);
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
