// # Wrapper for common Vulkan procedures.
// @PENGUINLIONG
#pragma once
#include <vulkan/vulkan.h>

namespace liong {
namespace vk {

extern VkInstance create_inst(uint32_t api_ver);
extern void destroy_inst(VkInstance inst);

extern std::vector<VkPhysicalDevice> collect_physdevs(VkInstance inst);
extern VkPhysicalDeviceProperties get_physdev_prop(const VkPhysicalDevice& physdev);

} // namespace vk
} // namespace liong
