// # Vulkan implementation of HAL
// @PENGUINLIONG
#pragma once

#include <array>
#include <map>
#include <memory>
#include <chrono>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#define HAL_IMPL_NAMESPACE vk
#include "gft/hal/scoped-hal.hpp"
#include "gft/vk-sys.hpp"

namespace liong {
namespace vk {

inline VkFormat fmt2vk(fmt::Format fmt, fmt::ColorSpace cspace) {
  using namespace fmt;
  switch (fmt) {
  case L_FORMAT_R8G8B8A8_UNORM_PACK32:
    if (cspace == L_COLOR_SPACE_SRGB) {
      return VK_FORMAT_R8G8B8A8_SRGB;
    } else {
      return VK_FORMAT_R8G8B8A8_UNORM;
    }
  case L_FORMAT_B8G8R8A8_UNORM_PACK32:
    if (cspace == L_COLOR_SPACE_SRGB) {
      return VK_FORMAT_B8G8R8A8_SRGB;
    } else {
      return VK_FORMAT_B8G8R8A8_UNORM;
    }
  case L_FORMAT_B10G11R11_UFLOAT_PACK32: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
  case L_FORMAT_R16G16B16A16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
  case L_FORMAT_R32_SFLOAT: return VK_FORMAT_R32_SFLOAT;
  case L_FORMAT_R32G32_SFLOAT: return VK_FORMAT_R32G32_SFLOAT;
  case L_FORMAT_R32G32B32A32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
  default: panic("unrecognized pixel format");
  }
  return VK_FORMAT_UNDEFINED;
}
inline VkFormat depth_fmt2vk(fmt::DepthFormat fmt) {
  using namespace fmt;
  switch (fmt) {
  case L_DEPTH_FORMAT_D16_UNORM: return VK_FORMAT_D16_UNORM;
  case L_DEPTH_FORMAT_D32_SFLOAT: return VK_FORMAT_D32_SFLOAT;
  default: panic("unsupported depth format");
  }
  return VK_FORMAT_UNDEFINED;
}
inline VkColorSpaceKHR cspace2vk(fmt::ColorSpace cspace) {
  using namespace fmt;
  switch (cspace) {
  case L_COLOR_SPACE_SRGB: return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  default: panic("unsupported color space");
  }
}

struct InstancePhysicalDeviceDetail {
  VkPhysicalDevice physdev;
  VkPhysicalDeviceProperties prop;
  VkPhysicalDeviceFeatures feat;
  VkPhysicalDeviceMemoryProperties mem_prop;
  std::vector<VkQueueFamilyProperties> qfam_props;
  std::map<std::string, uint32_t> ext_props;
  std::string desc;
};
struct Instance {
  uint32_t api_ver;
  sys::InstanceRef inst;
  std::vector<InstancePhysicalDeviceDetail> physdev_details;
  bool is_imported;
};
extern void initialize(uint32_t api_ver, VkInstance inst);
extern const Instance& get_inst();



enum SubmitType {
  L_SUBMIT_TYPE_ANY,
  L_SUBMIT_TYPE_COMPUTE,
  L_SUBMIT_TYPE_GRAPHICS,
  L_SUBMIT_TYPE_TRANSFER,
  L_SUBMIT_TYPE_PRESENT,
};
struct TransactionSubmitDetail {
  SubmitType submit_ty;
  VkCommandPool cmd_pool;
  VkCommandBuffer cmdbuf;
  VkQueue queue;
  sys::SemaphoreRef wait_sema;
  sys::SemaphoreRef signal_sema;
  bool is_submitted;
};
struct Transaction {
  const Context* ctxt;
  std::vector<TransactionSubmitDetail> submit_details;
  std::vector<sys::FenceRef> fences;
};



struct ContextSubmitDetail {
  uint32_t qfam_idx;
  VkQueue queue;
};
struct Context {
  std::string label;
  uint32_t iphysdev;
  sys::DeviceRef dev;
  VkSurfaceKHR surf;
  std::map<SubmitType, ContextSubmitDetail> submit_details;
  std::map<ImageSampler, VkSampler> img_samplers;
  std::map<DepthImageSampler, VkSampler> depth_img_samplers;
  VmaAllocator allocator;

  inline VkPhysicalDevice physdev() const {
    return get_inst().physdev_details.at(iphysdev).physdev;
  }
  inline const VkPhysicalDeviceProperties& physdev_prop() const {
    return get_inst().physdev_details.at(iphysdev).prop;
  }
  inline const VkPhysicalDeviceFeatures& physdev_feat() const {
    return get_inst().physdev_details.at(iphysdev).feat;
  }
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



struct SwapchainDynamicDetail {
  uint32_t width;
  uint32_t height;
  std::vector<Image> imgs;
  std::unique_ptr<uint32_t> img_idx;
};
struct Swapchain {
  const Context* ctxt;
  SwapchainConfig swapchain_cfg;
  VkSwapchainKHR swapchain;
  std::unique_ptr<SwapchainDynamicDetail> dyn_detail;
};



struct RenderPass {
  const Context* ctxt;
  uint32_t width;
  uint32_t height;
  VkRenderPass pass;
  RenderPassConfig pass_cfg;
  std::vector<VkClearValue> clear_values;
};



struct TaskDescriptorSetPoolItem {
  VkDescriptorPool desc_pool;
  VkDescriptorSet desc_set;
};
struct TaskDescriptorSetPoolItemRef {
  Task* task;
  TaskDescriptorSetPoolItem item;
  
  TaskDescriptorSetPoolItemRef(Task* task);
  ~TaskDescriptorSetPoolItemRef();
};
struct TaskResourceDetail {
  VkDescriptorSetLayout desc_set_layout;
  VkPipelineLayout pipe_layout;
  std::vector<ResourceType> rsc_tys;
  std::vector<VkDescriptorPoolSize> desc_pool_sizes;
  std::vector<TaskDescriptorSetPoolItem> desc_pool_items;
};
struct Task {
  std::string label;
  SubmitType submit_ty;
  const Context* ctxt;
  const RenderPass* pass; // Only for graphics task.
  VkPipeline pipe;
  DispatchSize workgrp_size; // Only for compute task.
  TaskResourceDetail rsc_detail;

  TaskDescriptorSetPoolItem acquire_desc_set();
  void release_desc_set(TaskDescriptorSetPoolItem&& item);
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
  std::unique_ptr<TaskDescriptorSetPoolItemRef> desc_set_item;
  DispatchSize workgrp_count;
};
struct InvocationGraphicsDetail {
  const Task* task;
  VkPipelineBindPoint bind_pt;
  std::unique_ptr<TaskDescriptorSetPoolItemRef> desc_set_item;
  std::vector<VkBuffer> vert_bufs;
  std::vector<VkDeviceSize> vert_buf_offsets;
  VkBuffer idx_buf;
  VkDeviceSize idx_buf_offset;
  uint32_t ninst;
  uint32_t nvert;
  IndexType idx_ty;
  uint32_t nidx;
};
struct InvocationRenderPassDetail {
  const RenderPass* pass;
  VkFramebuffer framebuf;
  bool is_baked;
  std::vector<const Invocation*> subinvokes;
};
struct InvocationPresentDetail {
  const Swapchain* swapchain;
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
  std::unique_ptr<InvocationPresentDetail> present_detail;
  std::unique_ptr<InvocationCompositeDetail> composite_detail;
  // Managed transitioning of resources referenced by invocation.
  InvocationTransitionDetail transit_detail;
  // Query pool for device-side timing, if required.
  VkQueryPool query_pool;
  // Baking artifacts. Currently we don't support baking render pass invocations
  // and those with switching submit types.
  std::unique_ptr<InvocationBakingDetail> bake_detail;
};

} // namespace vk
} // namespace liong
