#include "gft/vk.hpp"
#include "gft/log.hpp"

namespace liong {
namespace vk {

bool Transaction::create(const Invocation& invoke, InvocationSubmitTransactionConfig& cfg, Transaction& out) {
  const Context& ctxt = *invoke.ctxt;

  TransactionLike transact(ctxt, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  util::Timer timer {};
  timer.tic();
  invoke.record(transact);
  timer.toc();

  out.ctxt = &ctxt;
  out.submit_details = std::move(transact.submit_details);
  out.fences = std::move(transact.fences);
  L_DEBUG("created and submitted transaction for execution, command "
    "recording took ", timer.us(), "us");
  return true;  
}
Transaction::~Transaction() {
  if (fences.size() > 0) {
    L_DEBUG("destroyed transaction");
  }
}
bool Transaction::is_done() const {
  for (const auto& fence : fences) {
    VkResult err = vkGetFenceStatus(*ctxt->dev, fence->fence);
    if (err == VK_NOT_READY) {
      return false;
    } else {
      VK_ASSERT << err;
    }
  }
  return true;
}
void Transaction::wait() const {
  std::vector<VkFence> fences2(fences.size());
  for (size_t i = 0; i < fences.size(); ++i) {
    fences2.at(i) = fences.at(i)->fence;
  }

  util::Timer wait_timer {};
  wait_timer.tic();
  for (VkResult err;;) {
    err = vkWaitForFences(*ctxt->dev, fences2.size(), fences2.data(),
      VK_TRUE, SPIN_INTERVAL);
    if (err == VK_TIMEOUT) {
      // L_WARN("timeout after 3000ns");
    } else {
      VK_ASSERT << err;
      break;
    }
  }
  wait_timer.toc();

  L_DEBUG("command drain returned after ", wait_timer.us(), "us since the "
    "wait started (spin interval = ", SPIN_INTERVAL / 1000.0, "us");
}

} // namespace vk
} // namespace liong
