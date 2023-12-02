#pragma once
#include "gft/hal/transaction.hpp"
#include "gft/vk/vk-invocation.hpp"

namespace liong {
namespace vk {

struct VulkanTransaction;
typedef std::shared_ptr<VulkanTransaction> VulkanTransactionRef;

struct VulkanTransaction : public Transaction {
  VulkanContextRef ctxt;
  std::vector<TransactionSubmitDetail> submit_details;
  std::vector<sys::FenceRef> fences;

  static TransactionRef create(
    const InvocationRef& invoke,
    const TransactionConfig& cfg
  );
  VulkanTransaction(VulkanContextRef ctxt, TransactionInfo&& info);
  ~VulkanTransaction();

  virtual bool is_done() override final;
  virtual void wait() override final;
};

} // namespace vk
} // namespace liong
