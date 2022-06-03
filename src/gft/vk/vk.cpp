#include <map>
#include <algorithm>
#define VMA_IMPLEMENTATION
#include "gft/vk.hpp" // Defines `HAL_IMPL_NAMESPACE`.
#include "gft/assert.hpp"
#include "gft/util.hpp"
#include "gft/log.hpp"
#include "gft/hal/scoped-hal-impl.hpp" // `HAL_IMPL_AMESPACE` used here.

namespace liong {
namespace vk {


struct Instance {
  uint32_t api_ver;
  VkInstance inst;
  std::vector<VkPhysicalDevice> physdevs;
  std::vector<std::string> physdev_descs;
  bool is_borrowed;

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

  std::vector<VkPhysicalDevice> collect_physdevs(VkInstance inst) {
    uint32_t nphysdev {};
    VK_ASSERT << vkEnumeratePhysicalDevices(inst, &nphysdev, nullptr);
    std::vector<VkPhysicalDevice> physdevs;
    physdevs.resize(nphysdev);
    VK_ASSERT << vkEnumeratePhysicalDevices(inst, &nphysdev, physdevs.data());
    return physdevs;
  }
  std::vector<std::string> collect_physdev_descs(
    const std::vector<VkPhysicalDevice>& physdevs
  ) {
    std::vector<std::string> physdev_descs;
    for (auto i = 0; i < physdevs.size(); ++i) {
      VkPhysicalDeviceProperties physdev_prop;
      vkGetPhysicalDeviceProperties(physdevs[i], &physdev_prop);
      const char* dev_ty_lit;
      switch (physdev_prop.deviceType) {
      case VK_PHYSICAL_DEVICE_TYPE_OTHER: dev_ty_lit = "Other"; break;
      case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: dev_ty_lit = "Integrated GPU"; break;
      case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: dev_ty_lit = "Discrete GPU"; break;
      case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: dev_ty_lit = "Virtual GPU"; break;
      case VK_PHYSICAL_DEVICE_TYPE_CPU: dev_ty_lit = "CPU"; break;
      default: dev_ty_lit = "Unknown"; break;
      }
      auto desc = util::format(physdev_prop.deviceName, " (", dev_ty_lit, ", ",
        VK_VERSION_MAJOR(physdev_prop.apiVersion), ".",
        VK_VERSION_MINOR(physdev_prop.apiVersion), ")");
      physdev_descs.emplace_back(std::move(desc));
    }
    return phys_descs;
  }

  Instance(uint32_t api_ver) :
    Instance(api_ver, create_inst(), true) {}
  Instance(uint32_t api_ver, VkInstance inst) :
    Instance(api_ver, inst, false) {}
  Instance(uint32_t api_ver, VkInstance inst, bool is_owned) :
    api_ver(api_ver),
    inst(inst),
    physdevs(collect_physdevs(inst)),
    physdev_descs(collect_physdev_descs(physdevs))
    is_owned(is_owned) {
    log::info("vulkan backend initialized");
  }
  ~Instance() {
    if (is_owned) {
      vkDestroyInstance(inst_->inst, nullptr);
    }
    log::info("vulkan backend finalized");
  }
};
std::unique_ptr<Instance> inst_;

void initialize(uint32_t api_ver, VkInstance inst) {
  if (inst_ != nullptr) {
    log::warn("ignored redundant vulkan module initialization");
    return;
  }
  inst_ = std::make_unique<Instance>(api_ver, inst);
}
void initialize() {
  if (inst_ != nullptr) {
    log::warn("ignored redundant vulkan module initialization");
    return;
  }
  inst_ = std::make_unique<Instance>(VK_API_VERSION_1_0);
}
void finalize() {
  inst_ = nullptr;
}
std::string desc_dev(uint32_t idx) {
  L_ASSERT(inst_ != nullptr);
  const auto& physdev_descs = inst_->physdev_descs;
  return idx < physdev_descs.size() ? physdev_descs[idx] : std::string {};
}

} // namespace HAL_IMPL_NAMESPACE
} // namespace liong
