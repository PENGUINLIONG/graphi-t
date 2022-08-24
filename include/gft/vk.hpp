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

namespace liong {
namespace vk {

class VkException : public std::exception {
  std::string msg;
public:
  inline VkException(VkResult code) {
    switch (code) {
    case VK_ERROR_OUT_OF_HOST_MEMORY: msg = "out of host memory"; break;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: msg = "out of device memory"; break;
    case VK_ERROR_INITIALIZATION_FAILED: msg = "initialization failed"; break;
    case VK_ERROR_DEVICE_LOST: msg = "device lost"; break;
    case VK_ERROR_MEMORY_MAP_FAILED: msg = "memory map failed"; break;
    case VK_ERROR_LAYER_NOT_PRESENT: msg = "layer not supported"; break;
    case VK_ERROR_EXTENSION_NOT_PRESENT: msg = "extension may present"; break;
    case VK_ERROR_INCOMPATIBLE_DRIVER: msg = "incompatible driver"; break;
    case VK_ERROR_TOO_MANY_OBJECTS: msg = "too many objects"; break;
    case VK_ERROR_FORMAT_NOT_SUPPORTED: msg = "format not supported"; break;
    case VK_ERROR_FRAGMENTED_POOL: msg = "fragmented pool"; break;
    case VK_ERROR_OUT_OF_POOL_MEMORY: msg = "out of pool memory"; break;
    default: msg = std::string("unknown vulkan error: ") + std::to_string(code); break;
    }
  }

  inline const char* what() const noexcept override {
    return msg.c_str(); 
  }
};
struct VkAssert {
  inline const VkAssert& operator<<(VkResult code) const {
    if (code != VK_SUCCESS) { throw VkException(code); }
    return *this;
  }
};
#define VK_ASSERT (::liong::vk::VkAssert{})



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
  VkInstance inst;
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
  VkSemaphore wait_sema;
  VkSemaphore signal_sema;
  bool is_submitted;
};
struct Transaction {
  const Context* ctxt;
  std::vector<TransactionSubmitDetail> submit_details;
  std::vector<VkFence> fences;
};



struct ContextSubmitDetail {
  uint32_t qfam_idx;
  VkQueue queue;
};
struct DescriptorCounter {
  union {
    uint32_t key;
    struct {
      uint8_t nuniform_buf;
      uint8_t nstorage_buf;
      uint8_t nsampled_img;
      uint8_t nstorage_img;
    } counters;
  };
};
constexpr bool operator<(
  const DescriptorCounter& a,
  const DescriptorCounter& b
) {
  return a.key < b.key;
}
struct DescriptorPoolClass {
  const uint32_t POOL_SIZE_COE = 64;
  const uint32_t POOL_SIZE_ALIGN = 4;

  std::array<VkDescriptorPoolSize, 4> aligned_desc_pool_sizes;
  DescriptorCounter aligned_desc_counter;
};
struct DescriptorPoolEntry {
  VkDevice dev;
  VkDescriptorPool desc_pool;
  DescriptorPoolEntry(VkDevice dev, VkDescriptorPool desc_pool);
  ~DescriptorPoolEntry();
};
struct DescriptorSetEntry {
  VkDescriptorSetLayout desc_set_layout;
  VkDescriptorSet desc_set;
  std::shared_ptr<DescriptorPoolEntry> pool_entry;
};
struct ContextDescriptorPoolDetail {
  // Shader resource counter -> descriptor set layout.
  std::map<DescriptorCounter, VkDescriptorSetLayout> desc_set_layouts;
  // Descriptor set layout -> descriptor pool.
  std::map<VkDescriptorSetLayout, DescriptorPoolClass> desc_pool_classes;
  // Aligned resource counter -> instance.
  std::map<DescriptorCounter, std::vector<DescriptorSetEntry>> desc_sets;
};
struct Context {
  std::string label;
  uint32_t iphysdev;
  VkDevice dev;
  VkSurfaceKHR surf;
  std::map<SubmitType, ContextSubmitDetail> submit_details;
  std::map<ImageSampler, VkSampler> img_samplers;
  std::map<DepthImageSampler, VkSampler> depth_img_samplers;
  ContextDescriptorPoolDetail desc_pool_detail;
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

  DescriptorSetEntry acquire_desc_set(VkDescriptorSetLayout desc_set_layout);
  void release_desc_set(DescriptorSetEntry&& desc_set);
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



struct TaskResourceDetail {
  VkDescriptorSetLayout desc_set_layout;
  VkPipelineLayout pipe_layout;
  std::vector<ResourceType> rsc_tys;
};
struct Task {
  std::string label;
  SubmitType submit_ty;
  Context* ctxt;
  const RenderPass* pass; // Only for graphics task.
  VkPipeline pipe;
  DispatchSize workgrp_size; // Only for compute task.
  TaskResourceDetail rsc_detail;
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
  DescriptorSetEntry desc_set;
  DispatchSize workgrp_count;
};
struct InvocationGraphicsDetail {
  const Task* task;
  VkPipelineBindPoint bind_pt;
  DescriptorSetEntry desc_set;
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
  Context* ctxt;
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
