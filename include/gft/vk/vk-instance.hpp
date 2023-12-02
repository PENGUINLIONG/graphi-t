#pragma once
#include "gft/hal/instance.hpp"
#include "gft/vk-sys.hpp"

namespace liong {
namespace vk {

using namespace liong::hal;

struct VulkanInstance;
typedef std::shared_ptr<VulkanInstance> VulkanInstanceRef;

struct InstancePhysicalDeviceDetail {
  VkPhysicalDevice physdev;
  VkPhysicalDeviceProperties prop;
  VkPhysicalDeviceFeatures feat;
  VkPhysicalDeviceMemoryProperties mem_prop;
  std::vector<VkQueueFamilyProperties> qfam_props;
  std::map<std::string, uint32_t> ext_props;
  std::string desc;
};
struct VulkanInstance : public Instance {
  uint32_t api_ver;
  sys::InstanceRef inst;
  std::vector<InstancePhysicalDeviceDetail> physdev_details;
  bool is_imported;

  static VulkanInstanceRef create();
  static VulkanInstanceRef create(uint32_t api_ver, sys::InstanceRef&& inst);
  ~VulkanInstance();

  inline static VulkanInstanceRef from_hal(const InstanceRef& ref) {
    return std::static_pointer_cast<VulkanInstance>(ref);
  }

  virtual std::string describe_device(uint32_t device_index) override final;

  virtual ContextRef create_context(const ContextConfig& config) override final;
  virtual ContextRef create_context(const ContextWindowsConfig& config
  ) override final;
  virtual ContextRef create_context(const ContextAndroidConfig& config
  ) override final;
  virtual ContextRef create_context(const ContextMetalConfig& config
  ) override final;
};

} // namespace vk
} // namespace liong
