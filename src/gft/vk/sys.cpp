#include "gft/assert.hpp"
#include "gft/log.hpp"
#include "gft/vk.hpp"
#include "sys.hpp"

namespace liong {
namespace vk {


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

std::vector<VkPhysicalDevice> collect_physdevs(VkInstance inst) {
  uint32_t nphysdev {};
  VK_ASSERT << vkEnumeratePhysicalDevices(inst, &nphysdev, nullptr);
  std::vector<VkPhysicalDevice> physdevs;
  physdevs.resize(nphysdev);
  VK_ASSERT << vkEnumeratePhysicalDevices(inst, &nphysdev, physdevs.data());
  return physdevs;
}

VkPhysicalDeviceProperties get_physdev_prop(const VkPhysicalDevice& physdev) {
  VkPhysicalDeviceProperties out;
  vkGetPhysicalDeviceProperties(physdev, &out);
  return out;
}


} // namespace vk
} // namespace liong
