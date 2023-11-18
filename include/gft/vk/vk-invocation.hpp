#pragma once
#include "gft/hal/invocation.hpp"
#include "gft/vk/vk-context.hpp"
#include "gft/vk/vk-render-pass.hpp"
#include "gft/vk/vk-swapchain.hpp"
#include "gft/vk/vk-task.hpp"

namespace liong {
namespace vk {

struct VulkanInvocation;
typedef std::shared_ptr<VulkanInvocation> VulkanInvocationRef;

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
  const VulkanContextRef ctxt;

  std::vector<TransactionSubmitDetail> submit_details;
  std::vector<sys::FenceRef> fences;
  VkCommandBufferLevel level;
  // Some invocations cannot be followedby subsequent invocations, e.g.
  // presentation.
  bool is_frozen;

  inline TransactionLike(const VulkanContextRef& ctxt, VkCommandBufferLevel level) :
    ctxt(ctxt), submit_details(), level(level), is_frozen(false) {}
};

struct InvocationTransitionDetail {
  std::vector<std::pair<BufferView, BufferUsage>> buf_transit;
  std::vector<std::pair<ImageView, ImageUsage>> img_transit;
  std::vector<std::pair<DepthImageView, DepthImageUsage>> depth_img_transit;

  inline void reg(BufferView buf_view, BufferUsage usage) {
    buf_transit.emplace_back(
      std::make_pair<BufferView, BufferUsage>(
        std::move(buf_view), std::move(usage)));
  }
  inline void reg(ImageView img_view, ImageUsage usage) {
    img_transit.emplace_back(
      std::make_pair<ImageView, ImageUsage>(
        std::move(img_view), std::move(usage)));
  }
  inline void reg(DepthImageView depth_img_view, DepthImageUsage usage) {
    depth_img_transit.emplace_back(
      std::make_pair<DepthImageView, DepthImageUsage>(
        std::move(depth_img_view), std::move(usage)));
  }
};
struct InvocationCopyBufferToBufferDetail {
  VkBufferCopy bc;
  sys::BufferRef src;
  sys::BufferRef dst;
};
struct InvocationCopyBufferToImageDetail {
  VkBufferImageCopy bic;
  sys::BufferRef src;
  sys::ImageRef dst;
};
struct InvocationCopyImageToBufferDetail {
  VkBufferImageCopy bic;
  sys::ImageRef src;
  sys::BufferRef dst;
};
struct InvocationCopyImageToImageDetail {
  VkImageCopy ic;
  sys::ImageRef src;
  sys::ImageRef dst;
};
struct InvocationComputeDetail {
  VulkanTaskRef task;
  VkPipelineBindPoint bind_pt;
  DescriptorSetPoolItem desc_set;
  DispatchSize workgrp_count;
};
struct InvocationGraphicsDetail {
  VulkanTaskRef task;
  VkPipelineBindPoint bind_pt;
  DescriptorSetPoolItem desc_set;
  std::vector<sys::BufferRef> vert_bufs;
  std::vector<VkDeviceSize> vert_buf_offsets;
  sys::BufferRef idx_buf;
  VkDeviceSize idx_buf_offset;
  uint32_t ninst;
  uint32_t nvert;
  IndexType idx_ty;
  uint32_t nidx;
};
struct InvocationRenderPassDetail {
  RenderPassRef pass;
  FramebufferPoolItem framebuf;
  std::vector<sys::ImageViewRef> attms;
  bool is_baked;
  std::vector<VulkanInvocationRef> subinvokes;
};
struct InvocationPresentDetail {
  VulkanSwapchainRef swapchain;
};
struct InvocationCompositeDetail {
  std::vector<VulkanInvocationRef> subinvokes;
};
struct InvocationBakingDetail {
  CommandPoolPoolItem cmd_pool;
  sys::CommandBufferRef cmdbuf;
};
struct VulkanInvocation : public Invocation {
  // Execution context of the invocation.
  VulkanContextRef ctxt;
  // Case-by-case implementations.
  std::unique_ptr<InvocationCopyBufferToBufferDetail> b2b_detail;
  std::unique_ptr<InvocationCopyBufferToImageDetail> b2i_detail;
  std::unique_ptr<InvocationCopyImageToBufferDetail> i2b_detail;
  std::unique_ptr<InvocationCopyImageToImageDetail> i2i_detail;
  std::unique_ptr<InvocationComputeDetail> comp_detail;
  std::unique_ptr<InvocationGraphicsDetail> graph_detail;
  std::unique_ptr<InvocationRenderPassDetail> pass_detail;
  std::unique_ptr<InvocationPresentDetail> present_detail;
  std::unique_ptr<InvocationCompositeDetail> composite_detail;
  // Managed transitioning of resources referenced by invocation.
  InvocationTransitionDetail transit_detail;
  // Query pool for device-side timing, if required.
  QueryPoolPoolItem query_pool;
  // Baking artifacts. Currently we don't support baking render pass invocations
  // and those with switching submit types.
  std::unique_ptr<InvocationBakingDetail> bake_detail;

  static InvocationRef create(const ContextRef& ctxt, const TransferInvocationConfig& cfg);
  static InvocationRef create(const TaskRef& task, const ComputeInvocationConfig& cfg);
  static InvocationRef create(const TaskRef& task, const GraphicsInvocationConfig& cfg);
  static InvocationRef create(const RenderPassRef& pass, const RenderPassInvocationConfig& cfg);
  static InvocationRef create(const ContextRef& ctxt, const CompositeInvocationConfig& cfg);
  static InvocationRef create(const SwapchainRef& swapchain, const PresentInvocationConfig& cfg);
  VulkanInvocation(const VulkanContextRef& ctxt, InvocationInfo &&info);
  ~VulkanInvocation();

  void record(TransactionLike& transact) const;

  virtual double get_time_us() override final;
  virtual void bake() override final;

  inline static VulkanInvocationRef from_hal(const InvocationRef& ref) {
    return std::static_pointer_cast<VulkanInvocation>(ref);
  }
};

} // namespace vk
} // namespace liong
