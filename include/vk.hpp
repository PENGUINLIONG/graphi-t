// # Vulkan implementation of HAL
// @PENGUINLIONG
#pragma once
#include <array>
#include <map>
#include <chrono>
#include <vulkan/vulkan.h>
#define HAL_IMPL_NAMESPACE vk
#include "scoped-hal.hpp"
#undef HAL_IMPL_NAMESPACE

namespace liong {

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
#define VK_ASSERT (::liong::VkAssert{})



namespace vk {

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
  VkPhysicalDeviceProperties physdev_prop;
  std::vector<ContextSubmitDetail> submit_details;
  std::map<uint32_t, uint32_t> submit_detail_idx_by_submit_ty;
  std::array<std::vector<uint32_t>, 4> mem_ty_idxs_by_host_access;
  // Costless sampler to utilize L1 cache on old mobile platform.
  VkSampler fast_samp;
  ContextConfig ctxt_cfg;

  inline size_t get_queue_rsc_idx(SubmitType submit_ty) const {
    auto it = submit_detail_idx_by_submit_ty.find((uint32_t)submit_ty);
    liong::assert(it != submit_detail_idx_by_submit_ty.end(),
      "submit type ", submit_ty, " is not available");
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
};

struct Buffer {
  const Context* ctxt; // Lifetime bound.
  VkDeviceMemory devmem;
  VkBuffer buf;
  BufferConfig buf_cfg;
};

struct Image {
  const Context* ctxt; // Lifetime bound.
  VkDeviceMemory devmem;
  VkImage img;
  VkImageView img_view;
  ImageConfig img_cfg;
  bool is_staging_img;
};

struct WorkgroupSizeSpecializationDetail {
  uint32_t x_spec_id;
  uint32_t y_spec_id;
  uint32_t z_spec_id;
};
struct Task {
  const Context* ctxt;
  VkDescriptorSetLayout desc_set_layout;
  VkPipelineLayout pipe_layout;
  VkPipeline pipe;
  VkRenderPass pass;
  std::vector<VkShaderModule> shader_mods;
  std::vector<VkDescriptorPoolSize> desc_pool_sizes;
  std::string label;
  WorkgroupSizeSpecializationDetail workgrp_spec_detail;
};

struct Framebuffer {
  const Context* ctxt;
  const Task* task;
  const Image* img;
  VkRect2D viewport;
  VkFramebuffer framebuf;
  VkClearValue clear_value;
};

struct ResourcePool {
  const Context* ctxt;
  VkDescriptorPool desc_pool;
  VkDescriptorSet desc_set;
};

struct TransactionRenderPassDetail {
  VkRenderPass pass;
  VkFramebuffer framebuf;
  VkExtent2D render_area;
  VkClearValue clear_value;
};
struct TransactionSubmitDetail {
  SubmitType submit_ty;
  VkQueue queue;
  uint32_t qfam_idx;
  VkCommandBuffer cmdbuf;
  // If the `pass` member is not null, then there should be only one submit
  // detail in `submit_details` containing all the rendering command in the
  // render pass.
  TransactionRenderPassDetail pass_detail;
};
struct CommandDrain {
  const Context* ctxt;
  std::array<VkCommandPool, 2> cmd_pools;
  std::vector<VkSemaphore> semas;
  VkFence fence;
  // The time command buffer is written with any command.
  std::chrono::high_resolution_clock::time_point dirty_since;
  // The timne command buffer is submitted for execution.
  std::chrono::high_resolution_clock::time_point submit_since;
};

struct Transaction {
  std::string label;
  const Context* ctxt;
  std::array<VkCommandPool, 2> cmd_pools;
  std::vector<TransactionSubmitDetail> submit_details;
};

struct Timestamp {
  const Context* ctxt;
  VkQueryPool query_pool;
};

} // namespace vk

} // namespace liong
