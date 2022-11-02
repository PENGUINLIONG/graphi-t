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
#include "gft/pool.hpp"

namespace liong {
namespace vk {

const uint32_t SPIN_INTERVAL = 30'000;

inline VkFormat fmt2vk(fmt::Format fmt, fmt::ColorSpace cspace) {
  using namespace fmt;
  switch (fmt) {
  case L_FORMAT_R8G8B8A8_UNORM:
    if (cspace == L_COLOR_SPACE_SRGB) {
      return VK_FORMAT_R8G8B8A8_SRGB;
    } else {
      return VK_FORMAT_R8G8B8A8_UNORM;
    }
  case L_FORMAT_B8G8R8A8_UNORM:
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

typedef pool::Pool<SubmitType, sys::CommandPoolRef> CommandPoolPool;
typedef pool::PoolItem<SubmitType, sys::CommandPoolRef> CommandPoolPoolItem;

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
  const Context* ctxt;
  std::vector<TransactionSubmitDetail> submit_details;
  std::vector<sys::FenceRef> fences;
  VkCommandBufferLevel level;
  // Some invocations cannot be followedby subsequent invocations, e.g.
  // presentation.
  bool is_frozen;

  inline TransactionLike(const Context& ctxt, VkCommandBufferLevel level) :
    ctxt(&ctxt), submit_details(), level(level), is_frozen(false) {}
};
struct Transaction : public Transaction_ {
  const Context* ctxt;
  std::vector<TransactionSubmitDetail> submit_details;
  std::vector<sys::FenceRef> fences;

  static bool create(const Invocation& invoke, InvocationSubmitTransactionConfig& cfg, Transaction& out);
  ~Transaction();

  virtual bool is_done() const override final;
  virtual void wait() const override final;
};



struct DescriptorSetKey {
  std::string inner;

  static DescriptorSetKey create(const std::vector<ResourceType>& rsc_tys);

  inline friend bool operator<(const DescriptorSetKey& a, const DescriptorSetKey& b) {
    return a.inner < b.inner;
  }
};
typedef pool::Pool<DescriptorSetKey, sys::DescriptorSetRef> DescriptorSetPool;
typedef pool::PoolItem<DescriptorSetKey, sys::DescriptorSetRef> DescriptorSetPoolItem;

typedef pool::Pool<int, sys::QueryPoolRef> QueryPoolPool;
typedef pool::PoolItem<int, sys::QueryPoolRef> QueryPoolPoolItem;

struct ContextSubmitDetail {
  uint32_t qfam_idx;
  VkQueue queue;
};
struct ContextDescriptorSetDetail {
  std::map<DescriptorSetKey, sys::DescriptorSetLayoutRef> desc_set_layouts;
  // Descriptor pools to hold references.
  std::vector<sys::DescriptorPoolRef> desc_pools;
  DescriptorSetPool desc_set_pool;
};
struct Context : public Context_ {
  std::string label;
  uint32_t iphysdev;
  sys::DeviceRef dev;
  sys::SurfaceRef surf;
  std::map<SubmitType, ContextSubmitDetail> submit_details;
  std::map<ImageSampler, sys::SamplerRef> img_samplers;
  std::map<DepthImageSampler, sys::SamplerRef> depth_img_samplers;
  ContextDescriptorSetDetail desc_set_detail;
  CommandPoolPool cmd_pool_pool;
  QueryPoolPool query_pool_pool;
  sys::AllocatorRef allocator;

  static bool create(const Instance& inst, const ContextConfig& cfg, Context& out);
  static bool create(const Instance& inst, const ContextWindowsConfig& cfg, Context& out);
  static bool create(const Instance& inst, const ContextAndroidConfig& cfg, Context& out);
  static bool create(const Instance& inst, const ContextMetalConfig& cfg, Context& out);
  ~Context();

  inline VkPhysicalDevice physdev() const {
    return get_inst().physdev_details.at(iphysdev).physdev;
  }
  inline const VkPhysicalDeviceProperties& physdev_prop() const {
    return get_inst().physdev_details.at(iphysdev).prop;
  }
  inline const VkPhysicalDeviceFeatures& physdev_feat() const {
    return get_inst().physdev_details.at(iphysdev).feat;
  }

  sys::DescriptorSetLayoutRef get_desc_set_layout(const std::vector<ResourceType>& rsc_tys);
  DescriptorSetPoolItem acquire_desc_set(const std::vector<ResourceType>& rsc_tys);

  CommandPoolPoolItem acquire_cmd_pool(SubmitType submit_ty);

  QueryPoolPoolItem acquire_query_pool();
};



struct BufferDynamicDetail {
  VkPipelineStageFlags stage;
  VkAccessFlags access;
};
struct Buffer : public Buffer_ {
  const Context* ctxt; // Lifetime bound.
  sys::BufferRef buf;
  BufferConfig buf_cfg;
  BufferDynamicDetail dyn_detail;

  static bool create(const Context& ctxt, const BufferConfig& cfg, Buffer& out);
  ~Buffer();

  virtual const BufferConfig& cfg() const override final;

  virtual void* map(MemoryAccess map_access) override final;
  virtual void unmap(void* mapped) override final;

  virtual BufferView view(size_t offset, size_t size) const override final;
};



struct ImageDynamicDetail {
  VkPipelineStageFlags stage;
  VkAccessFlags access;
  VkImageLayout layout;
};
struct Image : public Image_ {
  const Context* ctxt; // Lifetime bound.
  sys::ImageRef img;
  sys::ImageViewRef img_view;
  ImageConfig img_cfg;
  ImageDynamicDetail dyn_detail;

  static bool create(const Context& ctxt, const ImageConfig& cfg, Image& out);
  ~Image();

  virtual const ImageConfig& cfg() const override final;
  virtual ImageView view(
    uint32_t x_offset,
    uint32_t y_offset,
    uint32_t z_offset,
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    ImageSampler sampler
  ) const override final;
};



struct DepthImageDynamicDetail {
  VkPipelineStageFlags stage;
  VkAccessFlags access;
  VkImageLayout layout;
};
struct DepthImage : public DepthImage_ {
  const Context* ctxt; // Lifetime bound.
  sys::ImageRef img;
  sys::ImageViewRef img_view;
  DepthImageConfig depth_img_cfg;
  DepthImageDynamicDetail dyn_detail;

  static bool create(const Context& ctxt, const DepthImageConfig& cfg, DepthImage& out);
  ~DepthImage();

  virtual const DepthImageConfig& cfg() const override final;
  virtual DepthImageView view(
    uint32_t x_offset,
    uint32_t y_offset,
    uint32_t width,
    uint32_t height,
    DepthImageSampler sampler
  ) const override final;
};



struct SwapchainDynamicDetail {
  uint32_t width;
  uint32_t height;
  std::vector<Image> imgs;
  std::unique_ptr<uint32_t> img_idx;
};
struct Swapchain : public Swapchain_ {
  const Context* ctxt;
  SwapchainConfig swapchain_cfg;
  sys::SwapchainRef swapchain;
  std::unique_ptr<SwapchainDynamicDetail> dyn_detail;

  static bool create(const Context& ctxt, const SwapchainConfig& cfg, Swapchain& out);
  ~Swapchain();

  virtual const SwapchainConfig& cfg() const;

  virtual const Image& get_img() const;
};



struct FramebufferKey {
  std::string inner;

  static FramebufferKey create(
    const RenderPass& pass,
    const std::vector<ResourceView>& rsc_views);

  friend inline bool operator<(const FramebufferKey& a, const FramebufferKey& b) {
    return a.inner < b.inner;
  }
};

typedef pool::Pool<FramebufferKey, sys::FramebufferRef> FramebufferPool;
typedef pool::PoolItem<FramebufferKey, sys::FramebufferRef> FramebufferPoolItem;
struct RenderPass {
  const Context* ctxt;
  uint32_t width;
  uint32_t height;
  sys::RenderPassRef pass;
  RenderPassConfig pass_cfg;
  std::vector<VkClearValue> clear_values;

  static bool create(const Context& ctxt, const RenderPassConfig& cfg, RenderPass& out);
  ~RenderPass();

  FramebufferPool framebuf_pool;
  FramebufferPoolItem acquire_framebuf(const std::vector<ResourceView>& attms);
};



struct TaskResourceDetail {
  sys::PipelineLayoutRef pipe_layout;
  std::vector<ResourceType> rsc_tys;
};
struct Task {
  std::string label;
  SubmitType submit_ty;
  const Context* ctxt;
  const RenderPass* pass; // Only for graphics task.
  sys::PipelineRef pipe;
  DispatchSize workgrp_size; // Only for compute task.
  TaskResourceDetail rsc_detail;

  static bool create(const Context& ctxt, const ComputeTaskConfig& cfg, Task& out);
  static bool create(const RenderPass& pass, const GraphicsTaskConfig& cfg, Task& out);
  ~Task();
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
  const Task* task;
  VkPipelineBindPoint bind_pt;
  DescriptorSetPoolItem desc_set;
  DispatchSize workgrp_count;
};
struct InvocationGraphicsDetail {
  const Task* task;
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
  const RenderPass* pass;
  FramebufferPoolItem framebuf;
  std::vector<sys::ImageViewRef> attms;
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
  CommandPoolPoolItem cmd_pool;
  sys::CommandBufferRef cmdbuf;
};
struct Invocation : public Invocation_ {
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
  QueryPoolPoolItem query_pool;
  // Baking artifacts. Currently we don't support baking render pass invocations
  // and those with switching submit types.
  std::unique_ptr<InvocationBakingDetail> bake_detail;

  static bool create(const Context& ctxt, const TransferInvocationConfig& cfg, Invocation& out);
  static bool create(const Task& task, const ComputeInvocationConfig& cfg, Invocation& out);
  static bool create(const Task& task, const GraphicsInvocationConfig& cfg, Invocation& out);
  static bool create(const RenderPass& pass, const RenderPassInvocationConfig& cfg, Invocation& out);
  static bool create(const Context& ctxt, const CompositeInvocationConfig& cfg, Invocation& out);
  static bool create(const Swapchain& swapchain, const PresentInvocationConfig& cfg, Invocation& out);
  ~Invocation();

  void record(TransactionLike& transact) const;

  virtual double get_time_us() const override final;
  virtual void bake() override final;
};

} // namespace vk
} // namespace liong
