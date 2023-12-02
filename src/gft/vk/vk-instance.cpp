#include "sys.hpp"
#include "gft/assert.hpp"
#include "gft/util.hpp"
#include "gft/log.hpp"
#include "gft/vk/vk-instance.hpp"
#include "gft/vk/vk-context.hpp"

namespace liong {
namespace vk {

void desc_physdev_prop(
  std::stringstream& ss,
  const VkPhysicalDeviceProperties& prop
) {
  const char* dev_ty_lit;
  switch (prop.deviceType) {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
      dev_ty_lit = "Other";
      break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
      dev_ty_lit = "Integrated GPU";
      break;
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
      dev_ty_lit = "Discrete GPU";
      break;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
      dev_ty_lit = "Virtual GPU";
      break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
      dev_ty_lit = "CPU";
      break;
    default:
      dev_ty_lit = "Unknown";
      break;
  }
  ss << util::format(
    prop.deviceName,
    " (",
    dev_ty_lit,
    ", ",
    VK_VERSION_MAJOR(prop.apiVersion),
    ".",
    VK_VERSION_MINOR(prop.apiVersion),
    ")"
  );
}
void desc_physdev_mem_prop(
  std::stringstream& ss,
  VkPhysicalDeviceMemoryProperties mem_prop
) {
  for (size_t i = 0; i < mem_prop.memoryHeapCount; ++i) {
    const VkMemoryHeap& heap = mem_prop.memoryHeaps[i];
    static const std::array<const char*, 1> flag_lits{
      "DEVICE_LOCAL",
    };
    std::vector<std::string> flags{};
    for (uint32_t j = 0; j < sizeof(heap.flags) * 8; ++j) {
      if (((heap.flags >> j) & 1) == 0) {
        continue;
      }
      if (j < flag_lits.size()) {
        flags.emplace_back(flag_lits[j]);
      } else {
        flags.emplace_back(util::format("(1 << ", j, ")"));
      }
    }
    std::string all_flags = flags.empty() ? "0" : util::join(" | ", flags);
    ss << "  memory heap #" << i << ": " << all_flags << std::endl;
  }
  for (size_t i = 0; i < mem_prop.memoryTypeCount; ++i) {
    const VkMemoryType& ty = mem_prop.memoryTypes[i];
    static const std::array<const char*, 6> flag_lits{
      "DEVICE_LOCAL",
      "HOST_VISIBLE",
      "HOST_COHERENT",
      "HOST_CACHED",
      "LAZILY_ALLOCATED",
      "PROTECTED",
    };
    std::vector<std::string> flags{};
    for (uint32_t j = 0; j < sizeof(ty.propertyFlags) * 8; ++j) {
      if (((ty.propertyFlags >> j) & 1) == 0) {
        continue;
      }
      if (j < flag_lits.size()) {
        flags.emplace_back(flag_lits[j]);
      } else {
        flags.emplace_back(util::format("(1 << ", j, ")"));
      }
    }
    std::string all_flags = flags.empty() ? "0" : util::join(" | ", flags);
    ss << "  memory type #" << i << " on heap #" << ty.heapIndex << ": "
       << all_flags;
  }
}

InstancePhysicalDeviceDetail make_physdev_detail(VkPhysicalDevice physdev) {
  auto prop = sys::get_physdev_prop(physdev);
  auto mem_prop = sys::get_physdev_mem_prop(physdev);

  std::stringstream ss{};
  desc_physdev_prop(ss, prop);
  desc_physdev_mem_prop(ss, mem_prop);

  InstancePhysicalDeviceDetail out{};
  out.physdev = physdev;
  out.prop = prop;
  out.mem_prop = mem_prop;
  out.feat = sys::get_physdev_feat(physdev);
  out.qfam_props = sys::collect_qfam_props(physdev);
  out.ext_props = sys::collect_physdev_ext_props(physdev);
  out.desc = ss.str();
  return out;
};

std::vector<InstancePhysicalDeviceDetail> collect_physdev_details(
  VkInstance inst
) {
  std::vector<VkPhysicalDevice> physdevs = sys::collect_physdevs(inst);
  std::vector<InstancePhysicalDeviceDetail> out{};
  out.reserve(physdevs.size());
  for (const auto& physdev : physdevs) {
    out.emplace_back(make_physdev_detail(physdev));
  }
  return out;
}

VulkanInstanceRef VulkanInstance::create(
  uint32_t api_ver,
  sys::InstanceRef&& inst
) {
  VulkanInstanceRef out = std::make_shared<VulkanInstance>();
  out->api_ver = api_ver;
  out->inst = std::move(inst);
  out->physdev_details = collect_physdev_details(inst->inst);
  out->is_imported = false;

  L_INFO("vulkan backend initialized with external instance");

  return out;
}
VulkanInstanceRef VulkanInstance::create() {
  sys::InstanceRef inst = sys::create_inst(VK_API_VERSION_1_0);
  std::vector<InstancePhysicalDeviceDetail> physdev_details =
    collect_physdev_details(inst->inst);

  VulkanInstanceRef out = std::make_shared<VulkanInstance>();
  out->api_ver = VK_API_VERSION_1_0;
  out->inst = std::move(inst);
  out->physdev_details = std::move(physdev_details);
  out->is_imported = true;

  L_INFO("vulkan backend initialized");

  return out;
}
VulkanInstance::~VulkanInstance() {}

void finalize() {
  L_INFO("vulkan backend finalized");
}
std::string VulkanInstance::describe_device(uint32_t device_index) {
  return device_index < physdev_details.size()
           ? physdev_details.at(device_index).desc
           : std::string{};
}

ContextRef VulkanInstance::create_context(const ContextConfig& cfg) {
  return VulkanContext::create(shared_from_this(), cfg);
}
ContextRef VulkanInstance::create_context(const ContextWindowsConfig& cfg) {
  return VulkanContext::create(shared_from_this(), cfg);
}
ContextRef VulkanInstance::create_context(const ContextAndroidConfig& cfg) {
  return VulkanContext::create(shared_from_this(), cfg);
}
ContextRef VulkanInstance::create_context(const ContextMetalConfig& cfg) {
  return VulkanContext::create(shared_from_this(), cfg);
}

}  // namespace vk
}  // namespace liong
