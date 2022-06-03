#include "gft/assert.hpp"
#include "gft/log.hpp"
#include "gft/vk.hpp"
#include "sys.hpp"

namespace liong {
namespace vk {
namespace sys {

// VkInstance
VkInstance create_inst(uint32_t api_ver) {
  VkApplicationInfo app_info {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.apiVersion = api_ver;
  app_info.pApplicationName = "TestbenchApp";
  app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  app_info.pEngineName = "GraphiT";
  app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);

  uint32_t ninst_ext = 0;
  VK_ASSERT << vkEnumerateInstanceExtensionProperties(nullptr, &ninst_ext, nullptr);

  std::vector<VkExtensionProperties> inst_exts;
  inst_exts.resize(ninst_ext);
  VK_ASSERT << vkEnumerateInstanceExtensionProperties(nullptr, &ninst_ext, inst_exts.data());

  uint32_t ninst_layer = 0;
  VK_ASSERT << vkEnumerateInstanceLayerProperties(&ninst_layer, nullptr);

  std::vector<VkLayerProperties> inst_layers;
  inst_layers.resize(ninst_layer);
  VK_ASSERT << vkEnumerateInstanceLayerProperties(&ninst_layer, inst_layers.data());

  // Enable all extensions by default.
  std::vector<const char*> inst_ext_names;
  inst_ext_names.reserve(ninst_ext);
  for (const auto& inst_ext : inst_exts) {
    inst_ext_names.emplace_back(inst_ext.extensionName);
  }
  log::debug("enabled instance extensions: ", util::join(", ", inst_ext_names));

  static std::vector<const char*> layers;
  for (const auto& inst_layer : inst_layers) {
    log::debug("found layer ", inst_layer.layerName);
#if !defined(NDEBUG)
    if (std::strcmp("VK_LAYER_KHRONOS_validation", inst_layer.layerName) == 0) {
      layers.emplace_back("VK_LAYER_KHRONOS_validation");
      log::debug("vulkan validation layer is enabled");
    }
#endif // !defined(NDEBUG)
  }

  VkInstanceCreateInfo ici {};
  ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  ici.pApplicationInfo = &app_info;
  ici.enabledExtensionCount = (uint32_t)inst_ext_names.size();
  ici.ppEnabledExtensionNames = inst_ext_names.data();
  ici.enabledLayerCount = (uint32_t)layers.size();
  ici.ppEnabledLayerNames = layers.data();

  VkInstance inst;
  VK_ASSERT << vkCreateInstance(&ici, nullptr, &inst);
  return inst;
}
void destroy_inst(VkInstance inst) {
  vkDestroyInstance(inst, nullptr);
}


// VkPhysicalDevice
std::vector<VkPhysicalDevice> collect_physdevs(VkInstance inst) {
  uint32_t nphysdev {};
  VK_ASSERT << vkEnumeratePhysicalDevices(inst, &nphysdev, nullptr);
  std::vector<VkPhysicalDevice> physdevs;
  physdevs.resize(nphysdev);
  VK_ASSERT << vkEnumeratePhysicalDevices(inst, &nphysdev, physdevs.data());
  return physdevs;
}

std::map<std::string, uint32_t> collect_physdev_ext_props(
  VkPhysicalDevice physdev
) {
  uint32_t ndev_ext = 0;
  VK_ASSERT << vkEnumerateDeviceExtensionProperties(physdev, nullptr, &ndev_ext,
    nullptr);
  std::vector<VkExtensionProperties> dev_exts;
  dev_exts.resize(ndev_ext);
  VK_ASSERT << vkEnumerateDeviceExtensionProperties(physdev, nullptr, &ndev_ext,
    dev_exts.data());

  std::map<std::string, uint32_t> out {};
  for (const auto& dev_ext : dev_exts) {
    std::string name = dev_ext.extensionName;
    uint32_t ver = dev_ext.specVersion;
    out.emplace(std::make_pair(std::move(name), ver));
  }
  return out;
}
VkPhysicalDeviceProperties get_physdev_prop(VkPhysicalDevice physdev) {
  VkPhysicalDeviceProperties out;
  vkGetPhysicalDeviceProperties(physdev, &out);
  return out;
}
VkPhysicalDeviceMemoryProperties get_physdev_mem_prop(VkPhysicalDevice physdev) {
  VkPhysicalDeviceMemoryProperties out;
  vkGetPhysicalDeviceMemoryProperties(physdev, &out);
  return out;
}
VkPhysicalDeviceFeatures get_physdev_feat(VkPhysicalDevice physdev) {
  VkPhysicalDeviceFeatures out;
  vkGetPhysicalDeviceFeatures(physdev, &out);
  return out;
}
std::vector<VkQueueFamilyProperties> collect_qfam_props(
  VkPhysicalDevice physdev
) {
  uint32_t nqfam_prop;
  vkGetPhysicalDeviceQueueFamilyProperties(physdev, &nqfam_prop, nullptr);
  std::vector<VkQueueFamilyProperties> qfam_props;
  qfam_props.resize(nqfam_prop);
  vkGetPhysicalDeviceQueueFamilyProperties(physdev,
    &nqfam_prop, qfam_props.data());
  return qfam_props;
}


// VkDevice
VkDevice create_dev(
  VkPhysicalDevice physdev,
  const std::vector<VkDeviceQueueCreateInfo> dqcis,
  const std::vector<const char*> enabled_ext_names,
  const VkPhysicalDeviceFeatures& enabled_feat
) {
  VkDeviceCreateInfo dci {};
  dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  dci.pEnabledFeatures = &enabled_feat;
  dci.queueCreateInfoCount = (uint32_t)dqcis.size();
  dci.pQueueCreateInfos = dqcis.data();
  dci.enabledExtensionCount = (uint32_t)enabled_ext_names.size();
  dci.ppEnabledExtensionNames = enabled_ext_names.data();

  VkDevice dev;
  VK_ASSERT << vkCreateDevice(physdev, &dci, nullptr, &dev);
  return dev;
}
void destroy_dev(VkDevice dev) {
  vkDestroyDevice(dev, nullptr);
}
VkQueue get_dev_queue(VkDevice dev, uint32_t qfam_idx, uint32_t queue_idx) {
  VkQueue queue;
  vkGetDeviceQueue(dev, qfam_idx, queue_idx, &queue);
  return queue;
}


// VkSampler
VkSampler create_sampler(
  VkDevice dev,
  VkFilter filter,
  VkSamplerMipmapMode mip_mode,
  float max_aniso,
  VkCompareOp cmp_op
) {
  VkSamplerCreateInfo sci {};
  sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sci.magFilter = filter;
  sci.minFilter = filter;
  sci.mipmapMode = mip_mode;
  sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  if (max_aniso > 1.0f) {
    sci.anisotropyEnable = VK_TRUE;
    sci.maxAnisotropy = max_aniso;
  }
  if (cmp_op != VK_COMPARE_OP_NEVER) {
    sci.compareEnable = VK_TRUE;
    sci.compareOp = cmp_op;
  }

  VkSampler sampler;
  VK_ASSERT << vkCreateSampler(dev, &sci, nullptr, &sampler);
  return sampler;
}
void destroy_sampler(VkDevice dev, VkSampler sampler) {
  vkDestroySampler(dev, sampler, nullptr);
}


} // namespace sys
} // namespace vk
} // namespace liong
