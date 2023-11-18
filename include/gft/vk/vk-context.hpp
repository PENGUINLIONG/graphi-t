#pragma once
#include "gft/hal/context.hpp"
#include "gft/vk/vk-instance.hpp"
#include "gft/pool.hpp"
#include "gft/vk-sys.hpp"

namespace liong {
namespace vk {

struct VulkanContext;
typedef std::shared_ptr<VulkanContext> VulkanContextRef;

typedef pool::Pool<SubmitType, sys::CommandPoolRef> CommandPoolPool;
typedef pool::PoolItem<SubmitType, sys::CommandPoolRef> CommandPoolPoolItem;

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
struct VulkanContext : public Context {
  VulkanInstanceRef inst;

  sys::DeviceRef dev;
  sys::SurfaceRef surf;
  std::map<SubmitType, ContextSubmitDetail> submit_details;
  std::map<ImageSampler, sys::SamplerRef> img_samplers;
  std::map<DepthImageSampler, sys::SamplerRef> depth_img_samplers;
  ContextDescriptorSetDetail desc_set_detail;
  CommandPoolPool cmd_pool_pool;
  QueryPoolPool query_pool_pool;
  sys::AllocatorRef allocator;

  static ContextRef create(const InstanceRef &inst, const ContextConfig& cfg);
  static ContextRef create(const InstanceRef &inst, const ContextWindowsConfig& cfg);
  static ContextRef create(const InstanceRef &inst, const ContextAndroidConfig& cfg);
  static ContextRef create(const InstanceRef &inst,const ContextMetalConfig &cfg);

  VulkanContext(VulkanInstanceRef inst, ContextInfo &&info);
  virtual ~VulkanContext();

  inline VkPhysicalDevice physdev() const {
    return inst->physdev_details.at(info.device_index).physdev;
  }
  inline const VkPhysicalDeviceProperties& physdev_prop() const {
    return inst->physdev_details.at(info.device_index).prop;
  }
  inline const VkPhysicalDeviceFeatures& physdev_feat() const {
    return inst->physdev_details.at(info.device_index).feat;
  }

  sys::DescriptorSetLayoutRef get_desc_set_layout(const std::vector<ResourceType>& rsc_tys);
  DescriptorSetPoolItem acquire_desc_set(const std::vector<ResourceType>& rsc_tys);

  CommandPoolPoolItem acquire_cmd_pool(SubmitType submit_ty);

  QueryPoolPoolItem acquire_query_pool();

  inline static VulkanContextRef from_hal(const ContextRef &ref) {
    return std::static_pointer_cast<VulkanContext>(ref);
  }
};

} // namespace vk
} // namespace liong
