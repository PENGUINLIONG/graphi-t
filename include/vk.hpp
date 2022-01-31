// # Vulkan implementation of HAL
// @PENGUINLIONG
#pragma once

#if GFT_WITH_VULKAN

#include <array>
#include <map>
#include <memory>
#include <chrono>
#include <vulkan/vulkan.h>
#define HAL_IMPL_NAMESPACE vk
#include "hal/scoped-hal.hpp"

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



struct ContextSubmitDetail {
  uint32_t qfam_idx;
  VkQueue queue;
};
struct Context {
  VkDevice dev;
  VkPhysicalDevice physdev;
  VkPhysicalDeviceProperties physdev_prop;
  std::vector<ContextSubmitDetail> submit_details;
  std::map<uint32_t, uint32_t> submit_detail_idx_by_submit_ty;
  std::array<std::vector<uint32_t>, 4> mem_ty_idxs_by_host_access;
  // Costless sampler to utilize L1 cache on old mobile platform.
  VkSampler fast_samp;
  ContextConfig ctxt_cfg;

  inline size_t get_queue_rsc_idx(SubmitType submit_ty) const {
    auto it = submit_detail_idx_by_submit_ty.find((uint32_t)submit_ty);
    assert(it != submit_detail_idx_by_submit_ty.end(), "submit type ",
      submit_ty, " is not available");
    return it->second;
  }
  inline const ContextSubmitDetail& get_submit_detail(
    SubmitType submit_ty
  ) const {
    auto i = get_queue_rsc_idx(submit_ty);
    assert(i < submit_details.size(), "unsupported submit type");
    return submit_details[i];
  }
  inline uint32_t get_submit_ty_qfam_idx(SubmitType submit_ty) const {
    if (submit_ty == L_SUBMIT_TYPE_ANY) {
      return VK_QUEUE_FAMILY_IGNORED;
    }
    auto isubmit_detail = get_queue_rsc_idx(submit_ty);
    return submit_details[isubmit_detail].qfam_idx;
  }
  inline VkQueue get_submit_ty_queue(SubmitType submit_ty) const {
    if (submit_ty == L_SUBMIT_TYPE_ANY) {
      return VK_NULL_HANDLE;
    }
    auto isubmit_detail = get_queue_rsc_idx(submit_ty);
    return submit_details[isubmit_detail].queue;
  }
};

struct BufferDynamicDetail {
  VkPipelineStageFlags stage;
  VkAccessFlags access;
};
struct Buffer {
  const Context* ctxt; // Lifetime bound.
  VkDeviceMemory devmem;
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
  VkDeviceMemory devmem;
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
  VkDeviceMemory devmem;
  size_t devmem_size;
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

struct WorkgroupSizeSpecializationDetail {
  uint32_t x_spec_id;
  uint32_t y_spec_id;
  uint32_t z_spec_id;
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
  WorkgroupSizeSpecializationDetail workgrp_spec_detail;
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
  std::vector<const Invocation*> invokes;
  bool is_baked;
};
struct Invocation {
  std::string label;
  SubmitType submit_ty;
  std::unique_ptr<InvocationComputeDetail> comp_detail;
  std::unique_ptr<InvocationGraphicsDetail> graph_detail;
  std::unique_ptr<InvocationRenderPassDetail> pass_detail;
  InvocationTransitionDetail transit_detail;
};

struct TransactionRenderPassDetail {
  VkRenderPass pass;
  VkFramebuffer framebuf;
  VkExtent2D render_area;
  VkClearValue clear_value;
};
struct TransactionSubmitDetail {
  const Context* ctxt;
  SubmitType submit_ty;
  VkCommandPool cmd_pool;
  VkCommandBuffer cmdbuf;
  VkSemaphore wait_sema;
  VkSemaphore signal_sema;
  // If the `pass` member is not null, then there should be only one submit
  // detail in `submit_details` containing all the rendering command in the
  // render pass.
  TransactionRenderPassDetail pass_detail;
};
struct CommandDrain {
  const Context* ctxt;
  std::vector<TransactionSubmitDetail> submit_details;
  VkFence fence;
};

struct Transaction {
  std::string label;
  const Context* ctxt;
  std::vector<TransactionSubmitDetail> submit_details;
};

struct Timestamp {
  const Context* ctxt;
  VkQueryPool query_pool;
};

} // namespace vk

} // namespace liong

#endif // GFT_WITH_VULKAN
