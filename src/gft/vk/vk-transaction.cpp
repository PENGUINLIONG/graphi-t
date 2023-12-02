#include "gft/vk/vk-transaction.hpp"
#include "gft/log.hpp"

namespace liong {
namespace vk {

VulkanTransaction::VulkanTransaction(
  VulkanContextRef ctxt,
  TransactionInfo&& info
) :
  Transaction(std::move(info)), ctxt(ctxt) {}

TransactionRef VulkanTransaction::create(
  const InvocationRef& invoke,
  const TransactionConfig& cfg
) {
  const VulkanInvocationRef& invoke_ = VulkanInvocation::from_hal(invoke);
  const VulkanContextRef& ctxt = VulkanContext::from_hal(invoke_->ctxt);

  TransactionLike transact(ctxt, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  util::Timer timer{};
  timer.tic();
  invoke_->record(transact);
  timer.toc();

  TransactionInfo info{};
  info.label = cfg.label;

  VulkanTransactionRef out =
    std::make_shared<VulkanTransaction>(ctxt, std::move(info));
  out->submit_details = std::move(transact.submit_details);
  out->fences = std::move(transact.fences);
  L_DEBUG(
    "created and submitted transaction for execution, command "
    "recording took ",
    timer.us(),
    "us"
  );
  return out;
}
VulkanTransaction::~VulkanTransaction() {
  if (fences.size() > 0) {
    L_DEBUG("destroyed transaction");
  }
}
bool VulkanTransaction::is_done() {
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
void VulkanTransaction::wait() {
  std::vector<VkFence> fences2(fences.size());
  for (size_t i = 0; i < fences.size(); ++i) {
    fences2.at(i) = fences.at(i)->fence;
  }

  util::Timer wait_timer{};
  wait_timer.tic();
  for (VkResult err;;) {
    err = vkWaitForFences(
      *ctxt->dev, fences2.size(), fences2.data(), VK_TRUE, SPIN_INTERVAL
    );
    if (err == VK_TIMEOUT) {
      // L_WARN("timeout after 3000ns");
    } else {
      VK_ASSERT << err;
      break;
    }
  }
  wait_timer.toc();

  L_DEBUG(
    "command drain returned after ",
    wait_timer.us(),
    "us since the "
    "wait started (spin interval = ",
    SPIN_INTERVAL / 1000.0,
    "us"
  );
}

}  // namespace vk
}  // namespace liong
