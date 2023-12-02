// # Wrapper for common Vulkan procedures.
// @PENGUINLIONG
#pragma once
#include <map>

#include <vulkan/vulkan.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "vulkan/vulkan_win32.h"
#endif // _WIN32

#if defined(ANDROID) || defined(__ANDROID__)
#include "vulkan/vulkan_android.h"
#endif // defined(ANDROID) || defined(__ANDROID__)

#if defined(__MACH__) && defined(__APPLE__)
#include "vulkan/vulkan_metal.h"
#endif // defined(__MACH__) && defined(__APPLE__)

#include "gft/vk-sys.hpp"
#include "gft/log.hpp"

namespace liong {
namespace vk {
namespace sys {

// VkInstance
extern sys::InstanceRef create_inst(uint32_t api_ver);

// VkPhysicalDevice
extern std::vector<VkPhysicalDevice> collect_physdevs(VkInstance inst);
extern std::map<std::string, uint32_t> collect_physdev_ext_props(
  VkPhysicalDevice physdev
);
extern VkPhysicalDeviceProperties get_physdev_prop(VkPhysicalDevice physdev);
extern VkPhysicalDeviceMemoryProperties get_physdev_mem_prop(
  VkPhysicalDevice physdev
);
extern VkPhysicalDeviceFeatures get_physdev_feat(VkPhysicalDevice physdev);
extern std::vector<VkQueueFamilyProperties> collect_qfam_props(
  VkPhysicalDevice physdev
);

// VkDevice
extern sys::DeviceRef create_dev(
  VkPhysicalDevice physdev,
  const std::vector<VkDeviceQueueCreateInfo> dqcis,
  const std::vector<const char*> enabled_ext_names,
  const VkPhysicalDeviceFeatures& enabled_feat
);
VkQueue get_dev_queue(VkDevice dev, uint32_t qfam_idx, uint32_t queue_idx);

// VkSampler
extern sys::SamplerRef create_sampler(
  VkDevice dev,
  VkFilter filter,
  VkSamplerMipmapMode mip_mode,
  float max_aniso,
  VkCompareOp cmp_op
);

// VkDescriptorSetLayout
extern sys::DescriptorSetLayoutRef create_desc_set_layout(
  VkDevice dev,
  const std::vector<VkDescriptorSetLayoutBinding>& dslbs
);

// VkPipelineLayout
extern sys::PipelineLayoutRef create_pipe_layout(
  VkDevice dev,
  VkDescriptorSetLayout desc_set_layout
);

// VkShaderModule
extern ShaderModuleRef create_shader_mod(
  VkDevice dev,
  const uint32_t* spv,
  size_t spv_size
);

// VkPipeline
extern sys::PipelineRef create_comp_pipe(
  VkDevice dev,
  VkPipelineLayout pipe_layout,
  const VkPipelineShaderStageCreateInfo& pssci
);
extern sys::PipelineRef create_graph_pipe(
  VkDevice dev,
  VkPipelineLayout pipe_layout,
  VkRenderPass pass,
  uint32_t width,
  uint32_t height,
  const VkPipelineInputAssemblyStateCreateInfo& piasci,
  const VkPipelineRasterizationStateCreateInfo& prsci,
  const std::array<VkPipelineShaderStageCreateInfo, 2> psscis
);


} // namespace sys

inline void request_name(
  const char* category,
  const char* expect,
  const char* actual,
  std::vector<const char*>& out
) {
  if (std::strcmp(expect, actual) == 0) {
    out.push_back(expect);
    L_INFO("enabled ", category, " ", expect);
  }
}
inline void request_instance_extension(
  const char* expect,
  const char* actual,
  std::vector<const char*>& out
) {
  request_name("instance extension", expect, actual, out);
}
inline void request_instance_layer(
  const char* expect,
  const char* actual,
  std::vector<const char*>& out
) {
  request_name("instance layer", expect, actual, out);
}
inline void request_device_extension(
  const char* expect,
  const char* actual,
  std::vector<const char*>& out
) {
  request_name("device extension", expect, actual, out);
}

} // namespace vk
} // namespace liong
