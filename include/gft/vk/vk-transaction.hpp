#pragma once
#include "gft/vk/transact.hpp"

namespace liong {
namespace vk {

struct TransactionSubmitDetail {
  SubmitType submit_ty;
  CommandPoolPoolItem cmd_pool;
  sys::CommandBufferRef cmdbuf;
  VkQueue queue;
  sys::SemaphoreRef wait_sema;
  sys::SemaphoreRef signal_sema;
  bool is_submitted;
};
struct TransactionLike {
  const std::shared_ptr<Context> ctxt;
  std::vector<TransactionSubmitDetail> submit_details;
  std::vector<sys::FenceRef> fences;
  VkCommandBufferLevel level;
  // Some invocations cannot be followedby subsequent invocations, e.g.
  // presentation.
  bool is_frozen;

  inline TransactionLike(const Context& ctxt, VkCommandBufferLevel level) :
    ctxt(ctxt.shared_from_this()), submit_details(), level(level), is_frozen(false) {}
};
struct Transaction_ : Transaction {
  const std::shared_ptr<Context> ctxt;
  std::vector<TransactionSubmitDetail> submit_details;
  std::vector<sys::FenceRef> fences;

  static bool create(const Invocation& invoke, InvocationSubmitTransactionConfig& cfg, Transaction& out);
  ~Transaction_();

  virtual bool is_done() const override final;
  virtual void wait() const override final;
};

} // namespace liong
} // namespace vk
