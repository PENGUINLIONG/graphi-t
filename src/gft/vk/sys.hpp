// # Wrapper for common Vulkan procedures.
// @PENGUINLIONG
#pragma once
#include <vulkan/vulkan.h>

namespace liong {
namespace vk {
namespace sys {

// VkInstance
extern VkInstance create_inst(uint32_t api_ver);
extern void destroy_inst(VkInstance inst);

// VkPhysicalDevice
extern std::vector<VkPhysicalDevice> collect_physdevs(VkInstance inst);
extern std::map<std::string, uint32_t> collect_physdev_ext_props(
  VkPhysicalDevice physdev
);
extern VkPhysicalDeviceProperties get_physdev_prop(VkPhysicalDevice physdev);
extern VkPhysicalDeviceMemoryProperties get_physdev_mem_prop(VkPhysicalDevice physdev);
extern VkPhysicalDeviceFeatures get_physdev_feat(VkPhysicalDevice physdev);
extern std::vector<VkQueueFamilyProperties> collect_qfam_props(
  VkPhysicalDevice physdev
);

// VkDevice
extern VkDevice create_dev(
  VkPhysicalDevice physdev,
  const std::vector<VkDeviceQueueCreateInfo> dqcis,
  const std::vector<const char*> enabled_ext_names,
  const VkPhysicalDeviceFeatures& enabled_feat
);
extern void destroy_dev(VkDevice dev);
VkQueue get_dev_queue(VkDevice dev, uint32_t qfam_idx, uint32_t queue_idx);

// VkSampler
extern VkSampler create_sampler(
  VkDevice dev,
  VkFilter filter,
  VkSamplerMipmapMode mip_mode,
  float max_aniso,
  VkCompareOp cmp_op
);
extern void destroy_sampler(VkDevice dev, VkSampler sampler);

} // namespace sys
} // namespace vk
} // namespace liong
