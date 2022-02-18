// # Vulkan implementation of HAL
// @PENGUINLIONG
#pragma once

#ifndef GFT_WITH_VULKAN
#define GFT_WITH_VULKAN 1
#endif

#if GFT_WITH_VULKAN

#include <array>
#include <map>
#include <memory>
#include <chrono>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#define HAL_IMPL_NAMESPACE vk
#include "gft/hal/scoped-hal.hpp"

namespace liong {

namespace vk {

class VkException : public std::exception {
  std::string msg;
public:
  VkException(VkResult code);

  const char* what() const noexcept override;
};
struct VkAssert {
  inline const VkAssert& operator<<(VkResult code) const {
    if (code != VK_SUCCESS) { throw VkException(code); }
    return *this;
  }
};



struct PhysicalDeviceStub {
  VkPhysicalDevice physdev;
  std::string desc;
};

extern VkInstance inst;
extern std::vector<VkPhysicalDevice> physdevs;
extern std::vector<std::string> physdev_descs;



enum SubmitType {
  L_SUBMIT_TYPE_ANY,
  L_SUBMIT_TYPE_COMPUTE,
  L_SUBMIT_TYPE_GRAPHICS,
  L_SUBMIT_TYPE_TRANSFER,
};
struct ContextSubmitDetail {
  uint32_t qfam_idx;
  VkQueue queue;
};
struct Context {
  VkDevice dev;
  VkPhysicalDevice physdev;
  VkPhysicalDeviceProperties physdev_prop;
  std::map<SubmitType, ContextSubmitDetail> submit_details;
  std::map<ImageSampler, VkSampler> img_samplers;
  std::map<DepthImageSampler, VkSampler> depth_img_samplers;
  VmaAllocator allocator;
  ContextConfig ctxt_cfg;
};



struct BufferDynamicDetail {
  VkPipelineStageFlags stage;
  VkAccessFlags access;
};
struct Buffer {
  const Context* ctxt; // Lifetime bound.
  VmaAllocation alloc;
  VkBuffer buf;
  BufferConfig buf_cfg;
  BufferDynamicDetail dyn_detail;
};



struct ImageDynamicDetail {
  VkPipelineStageFlags stage;
  VkAccessFlags access;
  VkImageLayout layout;
};
struct Image {
  const Context* ctxt; // Lifetime bound.
  VmaAllocation alloc;
  VkImage img;
  VkImageView img_view;
  ImageConfig img_cfg;
  bool is_staging_img;
  ImageDynamicDetail dyn_detail;
};



struct DepthImageDynamicDetail {
  VkPipelineStageFlags stage;
  VkAccessFlags access;
  VkImageLayout layout;
};
struct DepthImage {
  const Context* ctxt; // Lifetime bound.
  VmaAllocation alloc;
  VkImage img;
  VkImageView img_view;
  DepthImageConfig depth_img_cfg;
  DepthImageDynamicDetail dyn_detail;
};



struct RenderPass {
  const Context* ctxt;
  VkRect2D viewport;
  VkRenderPass pass;
  RenderPassConfig pass_cfg;
  std::vector<VkClearValue> clear_values;
};



struct Task {
  std::string label;
  SubmitType submit_ty;
  const Context* ctxt;
  const RenderPass* pass;
  VkDescriptorSetLayout desc_set_layout;
  VkPipelineLayout pipe_layout;
  VkPipeline pipe;
  std::vector<ResourceType> rsc_tys;
  std::vector<VkShaderModule> shader_mods;
  std::vector<VkDescriptorPoolSize> desc_pool_sizes;
  DispatchSize workgrp_size;
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
  VkBuffer src;
  VkBuffer dst;
};
struct InvocationCopyBufferToImageDetail {
  VkBufferImageCopy bic;
  VkBuffer src;
  VkImage dst;
};
struct InvocationCopyImageToBufferDetail {
  VkBufferImageCopy bic;
  VkImage src;
  VkBuffer dst;
};
struct InvocationCopyImageToImageDetail {
  VkImageCopy ic;
  VkImage src;
  VkImage dst;
};
struct InvocationComputeDetail {
  const Task* task;
  VkPipelineBindPoint bind_pt;
  VkDescriptorPool desc_pool;
  VkDescriptorSet desc_set;
  DispatchSize workgrp_count;
};
struct InvocationGraphicsDetail {
  const Task* task;
  VkPipelineBindPoint bind_pt;
  VkDescriptorPool desc_pool;
  VkDescriptorSet desc_set;
  std::vector<VkBuffer> vert_bufs;
  std::vector<VkDeviceSize> vert_buf_offsets;
  VkBuffer idx_buf;
  VkDeviceSize idx_buf_offset;
  uint32_t ninst;
  uint32_t nvert;
  uint32_t nidx;
};
struct InvocationRenderPassDetail {
  const RenderPass* pass;
  VkFramebuffer framebuf;
  bool is_baked;
  std::vector<const Invocation*> subinvokes;
};
struct InvocationCompositeDetail {
  std::vector<const Invocation*> subinvokes;
};
struct InvocationBakingDetail {
  VkCommandPool cmd_pool;
  VkCommandBuffer cmdbuf;
};
struct Invocation {
  std::string label;
  // Execution context of the invocation.
  const Context* ctxt;
  // Submit type of this invocation or the first non-any subinvocation.
  SubmitType submit_ty;
  std::unique_ptr<InvocationCopyBufferToBufferDetail> b2b_detail;
  std::unique_ptr<InvocationCopyBufferToImageDetail> b2i_detail;
  std::unique_ptr<InvocationCopyImageToBufferDetail> i2b_detail;
  std::unique_ptr<InvocationCopyImageToImageDetail> i2i_detail;
  std::unique_ptr<InvocationComputeDetail> comp_detail;
  std::unique_ptr<InvocationGraphicsDetail> graph_detail;
  std::unique_ptr<InvocationRenderPassDetail> pass_detail;
  std::unique_ptr<InvocationCompositeDetail> composite_detail;
  // Managed transitioning of resources referenced by invocation.
  InvocationTransitionDetail transit_detail;
  // Query pool for device-side timing, if required.
  VkQueryPool query_pool;
  // Baking artifacts. Currently we don't support baking render pass invocations
  // and those with switching submit types.
  std::unique_ptr<InvocationBakingDetail> bake_detail;
};



struct TransactionSubmitDetail {
  SubmitType submit_ty;
  VkCommandPool cmd_pool;
  VkCommandBuffer cmdbuf;
  VkSemaphore wait_sema;
  VkSemaphore signal_sema;
};
struct Transaction {
  const Invocation* invoke;
  std::vector<TransactionSubmitDetail> submit_details;
  VkFence fence;
};

} // namespace vk

} // namespace liong

#endif // GFT_WITH_VULKAN
