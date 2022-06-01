#include <set>
#include "gft/vk.hpp"
#include "gft/log.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "vulkan/vulkan_win32.h"
#endif // _WIN32

#if defined(ANDROID) || defined(__ANDROID__)
#include "vulkan/vulkan_android.h"
#endif // defined(ANDROID) || defined(__ANDROID__)

namespace liong {
namespace vk {

uint32_t api_ver = VK_API_VERSION_1_0;

VkSurfaceKHR _create_surf_windows(const ContextWindowsConfig& cfg) {
#if VK_KHR_win32_surface
  assert(cfg.dev_idx < physdevs.size(),
    "wanted vulkan device does not exists (#", cfg.dev_idx, " of ",
      physdevs.size(), " available devices)");
  auto physdev = physdevs[cfg.dev_idx];

  VkWin32SurfaceCreateInfoKHR wsci {};
  wsci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  wsci.hinstance = (HINSTANCE)cfg.hinst;
  wsci.hwnd = (HWND)cfg.hwnd;

  VkSurfaceKHR surf;
  VK_ASSERT << vkCreateWin32SurfaceKHR(inst, &wsci, nullptr, &surf);

  log::debug("created windows surface '", cfg.label, "'");
  return surf;
#else
  panic("windows surface cannot be created on current platform");
  return VK_NULL_HANDLE;
#endif // VK_KHR_win32_surface
}

VkSurfaceKHR _create_surf_android(const ContextAndroidConfig& cfg) {
#if VK_KHR_android_surface
  assert(cfg.dev_idx < physdevs.size(),
    "wanted vulkan device does not exists (#", cfg.dev_idx, " of ",
      physdevs.size(), " available devices)");
  auto physdev = physdevs[cfg.dev_idx];

  VkAndroidSurfaceCreateInfoKHR asci {};
  asci.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
  asci.window = (struct ANativeWindow* const)cfg.native_wnd;

  VkSurfaceKHR surf;
  VK_ASSERT << vkCreateAndroidSurfaceKHR(inst, &asci, nullptr, &surf);

  log::debug("created android surface '", cfg.label, "'");
  return surf;
#else
  panic("android surface cannot be created on current platform");
  return VK_NULL_HANDLE;
#endif // VK_KHR_android_surface
}
VkSampler _create_sampler(
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
Context _create_ctxt(
  const std::string& label,
  uint32_t dev_idx,
  VkSurfaceKHR surf
) {
  assert(dev_idx < physdevs.size(),
    "wanted vulkan device does not exists (#", dev_idx, " of ",
      physdevs.size(), " available devices)");
  auto physdev = physdevs[dev_idx];

  VkPhysicalDeviceFeatures feat;
  vkGetPhysicalDeviceFeatures(physdev, &feat);

  VkPhysicalDeviceProperties physdev_prop;
  vkGetPhysicalDeviceProperties(physdev, &physdev_prop);

  if (physdev_prop.limits.timestampComputeAndGraphics == VK_FALSE) {
    log::warn("context '", label, "' device does not support timestamps, "
      "the following command won't be available: WRITE_TIMESTAMP");
  }

  // Collect queue families and use as few queues as possible (for less sync).
  uint32_t nqfam_prop;
  vkGetPhysicalDeviceQueueFamilyProperties(physdev, &nqfam_prop, nullptr);
  std::vector<VkQueueFamilyProperties> qfam_props;
  qfam_props.resize(nqfam_prop);
  vkGetPhysicalDeviceQueueFamilyProperties(physdev,
    &nqfam_prop, qfam_props.data());

  struct QueueFamilyTrait {
    uint32_t qfam_idx;
    VkQueueFlags queue_flags;
  };
  std::map<uint32_t, std::vector<QueueFamilyTrait>> qfam_map {};
  for (uint32_t i = 0; i < qfam_props.size(); ++i) {
    const auto& qfam_prop = qfam_props[i];
    auto queue_flags = qfam_prop.queueFlags;
    if (qfam_prop.queueCount == 0) {
      log::warn("ignored queue family #", i, " with zero queue count");
    }

    std::vector<const char*> qfam_cap_lit{};
    if ((queue_flags & VK_QUEUE_GRAPHICS_BIT) != 0) {
      qfam_cap_lit.push_back("GRAPHICS");
    }
    if ((queue_flags & VK_QUEUE_COMPUTE_BIT) != 0) {
      qfam_cap_lit.push_back("COMPUTE");
    }
    if ((queue_flags & VK_QUEUE_TRANSFER_BIT) != 0) {
      qfam_cap_lit.push_back("TRANSFER");
    }
    if ((queue_flags & VK_QUEUE_SPARSE_BINDING_BIT) != 0) {
      qfam_cap_lit.push_back("SPARSE_BINDING");
    }
    if ((queue_flags & VK_QUEUE_PROTECTED_BIT) != 0) {
      qfam_cap_lit.push_back("PROTECTED");
    }
    log::debug("discovered queue families #", i, ": ",
      util::join(" | ", qfam_cap_lit));

    uint32_t nset_bit = util::count_set_bits(queue_flags);
    auto it = qfam_map.find(nset_bit);
    if (it == qfam_map.end()) {
      it = qfam_map.emplace_hint(it,
        std::make_pair(nset_bit, std::vector<QueueFamilyTrait>{}));
    }
    it->second.emplace_back(QueueFamilyTrait { i, queue_flags });
  }
  assert(!qfam_props.empty(),
    "cannot find any queue family on device #", dev_idx);

  struct SubmitTypeQueueRequirement {
    SubmitType submit_ty;
    const char* submit_ty_name;
    std::function<bool(const QueueFamilyTrait&)> pred;
  };
  std::vector<SubmitTypeQueueRequirement> submit_ty_reqs {
    SubmitTypeQueueRequirement {
      L_SUBMIT_TYPE_ANY,
      "ANY",
      [](const QueueFamilyTrait& qfam_trait) {
        return true;
      },
    },
    SubmitTypeQueueRequirement {
      L_SUBMIT_TYPE_GRAPHICS,
      "GRAPHICS",
      [](const QueueFamilyTrait& qfam_trait) {
        return qfam_trait.queue_flags & VK_QUEUE_GRAPHICS_BIT;
      },
    },
    SubmitTypeQueueRequirement {
      L_SUBMIT_TYPE_COMPUTE,
      "COMPUTE",
      [](const QueueFamilyTrait& qfam_trait) {
        return qfam_trait.queue_flags & VK_QUEUE_COMPUTE_BIT;
      },
    },
    SubmitTypeQueueRequirement {
      L_SUBMIT_TYPE_TRANSFER,
      "TRANSFER",
      [](const QueueFamilyTrait& qfam_trait) {
        return qfam_trait.queue_flags & (
          VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT
        );
      },
    },
    SubmitTypeQueueRequirement {
      L_SUBMIT_TYPE_PRESENT,
      "PRESENT",
      [&](const QueueFamilyTrait& qfam_trait) {
        if (surf == VK_NULL_HANDLE) { return false; }
        VkBool32 is_supported = VK_FALSE;
        VK_ASSERT << vkGetPhysicalDeviceSurfaceSupportKHR(physdev,
          qfam_trait.qfam_idx, surf, &is_supported);
        return is_supported == VK_TRUE;
      },
    },
  };

  std::map<SubmitType, uint32_t> queue_allocs;
  for (size_t i = 0; i < submit_ty_reqs.size(); ++i) {
    const SubmitTypeQueueRequirement& submit_ty_queue_req = submit_ty_reqs[i];

    queue_allocs[submit_ty_queue_req.submit_ty] = VK_QUEUE_FAMILY_IGNORED;
    uint32_t& qfam_idx_alloc = queue_allocs[submit_ty_queue_req.submit_ty];
    // Search from a combination of queue traits to queues of a single trait.
    for (auto it = qfam_map.rbegin(); it != qfam_map.rend(); ++it) {
      // We have already found a suitable queue family; no need to continue. 
      if (qfam_idx_alloc != VK_QUEUE_FAMILY_IGNORED) { break; }

      // Otherwise, look for a queue family that could satisfy all the required
      // flags.
      for (auto qfam_trait : it->second) {
        if (submit_ty_queue_req.pred(qfam_trait)) {
          qfam_idx_alloc = qfam_trait.qfam_idx;
          break;
        }
      }
    }

    if (qfam_idx_alloc == VK_QUEUE_FAMILY_IGNORED) {
      log::warn("cannot find a suitable queue family for ",
        submit_ty_queue_req.submit_ty_name);
    }
  }

  std::set<uint32_t> allocated_qfam_idxs;
  std::vector<VkDeviceQueueCreateInfo> dqcis;
  const float default_queue_prior = 1.0f;
  for (const auto& pair : queue_allocs) {
    uint32_t qfam_idx = pair.second;
    if (qfam_idx == VK_QUEUE_FAMILY_IGNORED) { continue; }

    auto it = allocated_qfam_idxs.find(qfam_idx);
    if (it == allocated_qfam_idxs.end()) {
      // The queue family has not been allocated before. Allocate it right now.
      VkDeviceQueueCreateInfo dqci {};
      dqci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      dqci.queueFamilyIndex = qfam_idx;
      dqci.queueCount = 1;
      dqci.pQueuePriorities = &default_queue_prior;

      dqcis.emplace_back(std::move(dqci));
      allocated_qfam_idxs.emplace_hint(it, qfam_idx);
    } else {
      // The queue family already has an instance of queue allocated. Reuse that
      // queue.
    }
  }

  uint32_t ndev_ext = 0;
  VK_ASSERT << vkEnumerateDeviceExtensionProperties(physdev, nullptr, &ndev_ext, nullptr);

  std::vector<VkExtensionProperties> dev_exts;
  dev_exts.resize(ndev_ext);
  VK_ASSERT << vkEnumerateDeviceExtensionProperties(physdev, nullptr, &ndev_ext, dev_exts.data());

  std::vector<const char*> dev_ext_names;
  dev_ext_names.reserve(ndev_ext);
  for (const auto& dev_ext : dev_exts) {
    dev_ext_names.emplace_back(dev_ext.extensionName);
  }
  log::debug("enabled device extensions: ", util::join(", ", dev_ext_names));

  VkDeviceCreateInfo dci {};
  dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  dci.pEnabledFeatures = &feat;
  dci.queueCreateInfoCount = static_cast<uint32_t>(dqcis.size());
  dci.pQueueCreateInfos = dqcis.data();
  dci.enabledExtensionCount = (uint32_t)dev_ext_names.size();
  dci.ppEnabledExtensionNames = dev_ext_names.data();

  VkDevice dev;
  VK_ASSERT << vkCreateDevice(physdev, &dci, nullptr, &dev);

  std::map<SubmitType, ContextSubmitDetail> submit_details;
  for (const auto& pair : queue_allocs) {
    SubmitType submit_ty = pair.first;
    uint32_t qfam_idx = pair.second;

    if (qfam_idx == VK_QUEUE_FAMILY_IGNORED) { continue; }

    VkQueue queue;
    vkGetDeviceQueue(dev, qfam_idx, 0, &queue);

    ContextSubmitDetail submit_detail {};
    submit_detail.qfam_idx = qfam_idx;
    submit_detail.queue = queue;
    submit_details.insert(std::make_pair<SubmitType, ContextSubmitDetail>(
      std::move(submit_ty), std::move(submit_detail)));
  }

  VkPhysicalDeviceMemoryProperties mem_prop;
  vkGetPhysicalDeviceMemoryProperties(physdev, &mem_prop);
  for (size_t i = 0; i < mem_prop.memoryHeapCount; ++i) {
    const VkMemoryHeap& heap = mem_prop.memoryHeaps[i];
    static const std::array<const char*, 1> flag_lits {
      "DEVICE_LOCAL",
    };
    std::vector<std::string> flags {};
    for (uint32_t j = 0; j < sizeof(heap.flags) * 8; ++j) {
      if (((heap.flags >> j) & 1) == 0) { continue; }
      if (j < flag_lits.size()) {
        flags.emplace_back(flag_lits[j]);
      } else {
        flags.emplace_back(util::format("(1 << ", j, ")"));
      }
    }
    std::string all_flags = flags.empty() ? "0" : util::join(" | ", flags);
    log::debug("memory heap #", i, ": ", all_flags);
  }
  for (size_t i = 0; i < mem_prop.memoryTypeCount; ++i) {
    const VkMemoryType& ty = mem_prop.memoryTypes[i];
    static const std::array<const char*, 6> flag_lits {
      "DEVICE_LOCAL",
      "HOST_VISIBLE",
      "HOST_COHERENT",
      "HOST_CACHED",
      "LAZILY_ALLOCATED",
      "PROTECTED",
    };
    std::vector<std::string> flags {};
    for (uint32_t j = 0; j < sizeof(ty.propertyFlags) * 8; ++j) {
      if (((ty.propertyFlags >> j) & 1) == 0) { continue; }
      if (j < flag_lits.size()) {
        flags.emplace_back(flag_lits[j]);
      } else {
        flags.emplace_back(util::format("(1 << ", j, ")"));
      }
    }
    std::string all_flags = flags.empty() ? "0" : util::join(" | ", flags);
    log::debug("memory type #", i, " on heap #", ty.heapIndex, ": ", all_flags);
  }

  std::map<ImageSampler, VkSampler> img_samplers {};
  img_samplers[L_IMAGE_SAMPLER_LINEAR] = _create_sampler(dev,
    VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.0f, VK_COMPARE_OP_NEVER);
  img_samplers[L_IMAGE_SAMPLER_NEAREST] = _create_sampler(dev,
    VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, 0.0f, VK_COMPARE_OP_NEVER);
  img_samplers[L_IMAGE_SAMPLER_ANISOTROPY_4] = _create_sampler(dev,
    VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 4.0f, VK_COMPARE_OP_NEVER);

  std::map<DepthImageSampler, VkSampler> depth_img_samplers {};
  depth_img_samplers[L_DEPTH_IMAGE_SAMPLER_LINEAR] = _create_sampler(dev,
    VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.0f, VK_COMPARE_OP_LESS);
  depth_img_samplers[L_DEPTH_IMAGE_SAMPLER_NEAREST] = _create_sampler(dev,
    VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, 0.0f, VK_COMPARE_OP_LESS);
  depth_img_samplers[L_DEPTH_IMAGE_SAMPLER_ANISOTROPY_4] = _create_sampler(dev,
    VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 4.0f, VK_COMPARE_OP_LESS);

  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.vulkanApiVersion = api_ver;
  allocatorInfo.physicalDevice = physdev;
  allocatorInfo.device = dev;
  allocatorInfo.instance = inst;

  VmaAllocator allocator;
  VK_ASSERT << vmaCreateAllocator(&allocatorInfo, &allocator);

  log::debug("created vulkan context '", label, "' on device #", dev_idx, ": ",
    physdev_descs[dev_idx]);
  return Context {
    label, dev, surf, physdev, std::move(physdev_prop),
    std::move(submit_details), img_samplers, depth_img_samplers, allocator
  };

}

Context create_ctxt(const ContextConfig& cfg) {
  if (inst == VK_NULL_HANDLE) { initialize(); }
  return _create_ctxt(cfg.label, cfg.dev_idx, VK_NULL_HANDLE);
}
Context create_ctxt_windows(const ContextWindowsConfig& cfg) {
  if (inst == VK_NULL_HANDLE) { initialize(); }
  VkSurfaceKHR surf = _create_surf_windows(cfg);
  return _create_ctxt(cfg.label, cfg.dev_idx, surf);
}
Context create_ctxt_android(const ContextAndroidConfig& cfg) {
  if (inst == VK_NULL_HANDLE) { initialize(); }
  VkSurfaceKHR surf = _create_surf_android(cfg);
  return _create_ctxt(cfg.label, cfg.dev_idx, surf);
}
void destroy_ctxt(Context& ctxt) {
  if (ctxt.surf != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(inst, ctxt.surf, nullptr);
  }
  if (ctxt.dev != VK_NULL_HANDLE) {
    for (const auto& samp : ctxt.img_samplers) {
      vkDestroySampler(ctxt.dev, samp.second, nullptr);
    }
    for (const auto& samp : ctxt.depth_img_samplers) {
      vkDestroySampler(ctxt.dev, samp.second, nullptr);
    }
    vmaDestroyAllocator(ctxt.allocator);
    vkDestroyDevice(ctxt.dev, nullptr);
    log::debug("destroyed vulkan context '", ctxt.label, "'");
  }
  ctxt = {};
}


} // namespace vk
} // namespace liong
