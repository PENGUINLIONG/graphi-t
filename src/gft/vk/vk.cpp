#include <map>
#include <algorithm>
#define VMA_IMPLEMENTATION
#include "gft/vk.hpp" // Defines `HAL_IMPL_NAMESPACE`.
#include "gft/assert.hpp"
#include "gft/util.hpp"
#include "gft/log.hpp"
#include "gft/hal/scoped-hal-impl.hpp" // `HAL_IMPL_AMESPACE` used here.
#include "sys.hpp"

namespace liong {
namespace vk {

std::unique_ptr<Instance> INST;

InstancePhysicalDeviceDetail make_physdev_detail(VkPhysicalDevice physdev) {
  auto prop = get_physdev_prop(physdev);

  const char* dev_ty_lit;
  switch (prop.deviceType) {
  case VK_PHYSICAL_DEVICE_TYPE_OTHER: dev_ty_lit = "Other"; break;
  case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: dev_ty_lit = "Integrated GPU"; break;
  case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: dev_ty_lit = "Discrete GPU"; break;
  case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: dev_ty_lit = "Virtual GPU"; break;
  case VK_PHYSICAL_DEVICE_TYPE_CPU: dev_ty_lit = "CPU"; break;
  default: dev_ty_lit = "Unknown"; break;
  }
  auto desc = util::format(prop.deviceName, " (", dev_ty_lit, ", ",
    VK_VERSION_MAJOR(prop.apiVersion), ".",
    VK_VERSION_MINOR(prop.apiVersion), ")");

  InstancePhysicalDeviceDetail out {};
  out.physdev = physdev;
  out.prop = prop;
  out.feat = get_physdev_feat(physdev);
  out.qfam_props = collect_qfam_props(physdev);
  out.desc = std::move(desc);
  return out;
};

std::vector<InstancePhysicalDeviceDetail> collect_physdev_details(
  VkInstance inst
) {
  std::vector<VkPhysicalDevice> physdevs = collect_physdevs(inst);
  std::vector<InstancePhysicalDeviceDetail> out {};
  out.reserve(physdevs.size());
  for (const auto& physdev : physdevs) {
    out.emplace_back(make_physdev_detail(physdev));
  }
  return out;
}

void initialize(uint32_t api_ver, VkInstance inst) {
  if (INST != nullptr) {
    log::warn("ignored redundant vulkan module initialization");
    return;
  }

  Instance out {};
  out.api_ver = VK_API_VERSION_1_0;
  out.inst = inst;
  out.physdev_details = collect_physdev_details(inst);
  out.is_imported = false;
  INST = std::make_unique<Instance>(out);
  log::info("vulkan backend initialized with external instance");
}
void initialize() {
  if (INST != nullptr) {
    log::warn("ignored redundant vulkan module initialization");
    return;
  }

  VkInstance inst = create_inst(VK_API_VERSION_1_0);
  std::vector<InstancePhysicalDeviceDetail> physdev_details =
    collect_physdev_details(inst);

  Instance out {};
  out.api_ver = VK_API_VERSION_1_0;
  out.inst = inst;
  out.physdev_details = std::move(physdev_details);
  out.is_imported = true;
  INST = std::make_unique<Instance>(out);
  log::info("vulkan backend initialized");
}
void finalize() {
  if (INST != nullptr) {
    destroy_inst(INST->inst);
    INST = nullptr;
    log::info("vulkan backend finalized");
  }
}
std::string desc_dev(uint32_t idx) {
  if (INST != nullptr) {
    const auto& physdev_details = INST->physdev_details;
    return idx < physdev_details.size() ?
      physdev_details.at(idx).desc : std::string {};
  } else {
    return {};
  }
}
const Instance& get_inst() {
  return *INST;
}

} // namespace vk
} // namespace liong
