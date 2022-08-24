#include <set>
#include "gft/vk.hpp"
#include "gft/log.hpp"
#include "gft/assert.hpp"
#include "sys.hpp"

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

namespace liong {
namespace vk {

VkSurfaceKHR _create_surf_windows(const ContextWindowsConfig& cfg) {
#if VK_KHR_win32_surface
  L_ASSERT(cfg.dev_idx < get_inst().physdev_details.size(),
    "wanted vulkan device does not exists (#", cfg.dev_idx, " of ",
    get_inst().physdev_details.size(), " available devices)");
  auto physdev = get_inst().physdev_details.at(cfg.dev_idx);

  VkWin32SurfaceCreateInfoKHR wsci {};
  wsci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  wsci.hinstance = (HINSTANCE)cfg.hinst;
  wsci.hwnd = (HWND)cfg.hwnd;

  VkSurfaceKHR surf;
  VK_ASSERT << vkCreateWin32SurfaceKHR(get_inst().inst, &wsci, nullptr, &surf);

  log::debug("created windows surface '", cfg.label, "'");
  return surf;
#else
  panic("windows surface cannot be created on current platform");
  return VK_NULL_HANDLE;
#endif // VK_KHR_win32_surface
}

VkSurfaceKHR _create_surf_android(const ContextAndroidConfig& cfg) {
#if VK_KHR_android_surface
  L_ASSERT(cfg.dev_idx < get_inst().physdev_details.size(),
    "wanted vulkan device does not exists (#", cfg.dev_idx, " of ",
      get_inst().physdev_details.size(), " available devices)");
  auto physdev = get_inst().physdev_details.at(cfg.dev_idx);

  VkAndroidSurfaceCreateInfoKHR asci {};
  asci.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
  asci.window = (struct ANativeWindow* const)cfg.native_wnd;

  VkSurfaceKHR surf;
  VK_ASSERT << vkCreateAndroidSurfaceKHR(get_inst().inst, &asci, nullptr, &surf);

  log::debug("created android surface '", cfg.label, "'");
  return surf;
#else
  panic("android surface cannot be created on current platform");
  return VK_NULL_HANDLE;
#endif // VK_KHR_android_surface
}

VkSurfaceKHR _create_surf_metal(const ContextMetalConfig& cfg) {
#if VK_EXT_metal_surface
  L_ASSERT(cfg.dev_idx < get_inst().physdev_details.size(),
    "wanted vulkan device does not exists (#", cfg.dev_idx, " of ",
    get_inst().physdev_details.size(), " available devices)");
  auto physdev = get_inst().physdev_details.at(cfg.dev_idx);

  VkMetalSurfaceCreateInfoEXT msci {};
  msci.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
  msci.pLayer = (const CAMetalLayer*)cfg.metal_layer;

  VkSurfaceKHR surf;
  VK_ASSERT << vkCreateMetalSurfaceEXT(get_inst().inst, &msci, nullptr, &surf);

  log::debug("created windows surface '", cfg.label, "'");
  return surf;
#else
  panic("metal surface cannot be created on current platform");
  return VK_NULL_HANDLE;
#endif // VK_EXT_metal_surface
}

DescriptorPoolEntry::DescriptorPoolEntry(
  VkDevice dev,
  VkDescriptorPool desc_pool
) : dev(dev), desc_pool(desc_pool) {}
DescriptorPoolEntry::~DescriptorPoolEntry() {
  sys::destroy_desc_pool(dev, desc_pool);
}
DescriptorSetEntry Context::acquire_desc_set(VkDescriptorSetLayout desc_set_layout) {
  auto& desc_pool_class = desc_pool_detail.desc_pool_classes.at(desc_set_layout);
  auto& desc_sets = desc_pool_detail.desc_sets[desc_pool_class.aligned_desc_counter];
  if (desc_sets.empty()) {
    VkDescriptorPool desc_pool = sys::create_desc_pool(dev,
      desc_pool_class.aligned_desc_pool_sizes, desc_pool_class.POOL_SIZE_COE);
    std::shared_ptr<DescriptorPoolEntry> desc_pool_entry =
      std::make_shared<DescriptorPoolEntry>(dev, desc_pool);

    std::vector<VkDescriptorSet> desc_sets2 =
      sys::allocate_desc_set(dev, desc_pool,
        desc_pool_class.aligned_desc_pool_sizes, desc_pool_class.POOL_SIZE_COE);
    for (VkDescriptorSet desc_set : desc_sets2) {
      DescriptorSetEntry desc_set_entry {};
      desc_set_entry.desc_set = desc_set;
      desc_set_entry.desc_set_layout = desc_set_layout;
      desc_set_entry.pool_entry = desc_pool_entry;
      desc_sets.emplace_back(std::move(desc_set_entry));
    }
  }
  DescriptorSetEntry desc_set = std::move(desc_sets.back());
  desc_sets.pop_back();
  return desc_set;
}
void Context::release_desc_set(DescriptorSetEntry&& desc_set_entry) {
  const DescriptorCounter& aligned_desc_counter =
    desc_pool_detail.desc_pool_classes.at(desc_set_entry.desc_set_layout)
      .aligned_desc_counter;
  desc_pool_detail.desc_sets.at(aligned_desc_counter).emplace_back(std::move(desc_set_entry));
}

Context _create_ctxt(
  const std::string& label,
  uint32_t dev_idx,
  VkSurfaceKHR surf
) {
  const Instance& inst = get_inst();

  L_ASSERT(dev_idx < inst.physdev_details.size(),
    "wanted vulkan device does not exists (#", dev_idx, " of ",
    inst.physdev_details.size(), " available devices)");
  const InstancePhysicalDeviceDetail& physdev_detail =
    inst.physdev_details.at(dev_idx);

  VkPhysicalDevice physdev = physdev_detail.physdev;
  const auto& prop = physdev_detail.prop;
  const auto& feat = physdev_detail.feat;
  const auto& qfam_props = physdev_detail.qfam_props;

  if (prop.limits.timestampComputeAndGraphics == VK_FALSE) {
    log::warn("context '", label, "' device does not support timestamps, "
      "the following command won't be available: WRITE_TIMESTAMP");
  }

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
  L_ASSERT(!qfam_props.empty(),
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

  std::vector<const char*> dev_exts;
  dev_exts.reserve(physdev_detail.ext_props.size());
  for (const auto& dev_ext : physdev_detail.ext_props) {
    dev_exts.emplace_back(dev_ext.first.c_str());
  }
  log::debug("enabled device extensions: ", util::join(", ", dev_exts));

  VkDevice dev = sys::create_dev(physdev_detail.physdev, dqcis, dev_exts, feat);

  std::map<SubmitType, ContextSubmitDetail> submit_details;
  for (const auto& pair : queue_allocs) {
    SubmitType submit_ty = pair.first;
    uint32_t qfam_idx = pair.second;

    if (qfam_idx == VK_QUEUE_FAMILY_IGNORED) { continue; }

    ContextSubmitDetail submit_detail {};
    submit_detail.qfam_idx = qfam_idx;
    submit_detail.queue = sys::get_dev_queue(dev, qfam_idx, 0);
    submit_details.insert(std::make_pair<SubmitType, ContextSubmitDetail>(
      std::move(submit_ty), std::move(submit_detail)));
  }

  std::map<ImageSampler, VkSampler> img_samplers {};
  img_samplers[L_IMAGE_SAMPLER_LINEAR] = sys::create_sampler(dev,
    VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.0f, VK_COMPARE_OP_NEVER);
  img_samplers[L_IMAGE_SAMPLER_NEAREST] = sys::create_sampler(dev,
    VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, 0.0f, VK_COMPARE_OP_NEVER);
  img_samplers[L_IMAGE_SAMPLER_ANISOTROPY_4] = sys::create_sampler(dev,
    VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 4.0f, VK_COMPARE_OP_NEVER);

  std::map<DepthImageSampler, VkSampler> depth_img_samplers {};
  depth_img_samplers[L_DEPTH_IMAGE_SAMPLER_LINEAR] = sys::create_sampler(dev,
    VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.0f, VK_COMPARE_OP_LESS);
  depth_img_samplers[L_DEPTH_IMAGE_SAMPLER_NEAREST] = sys::create_sampler(dev,
    VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, 0.0f, VK_COMPARE_OP_LESS);
  depth_img_samplers[L_DEPTH_IMAGE_SAMPLER_ANISOTROPY_4] = sys::create_sampler(dev,
    VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 4.0f, VK_COMPARE_OP_LESS);

  ContextDescriptorPoolDetail desc_pool_detail {};

  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.vulkanApiVersion = inst.api_ver;
  allocatorInfo.physicalDevice = physdev;
  allocatorInfo.device = dev;
  allocatorInfo.instance = inst.inst;

  VmaAllocator allocator;
  VK_ASSERT << vmaCreateAllocator(&allocatorInfo, &allocator);

  log::debug("created vulkan context '", label, "' on device #", dev_idx, ": ",
    inst.physdev_details.at(dev_idx).desc);
  return Context {
    label, dev_idx, dev, surf,
    std::move(submit_details), img_samplers, depth_img_samplers,
    desc_pool_detail, allocator
  };

}

Context create_ctxt(const ContextConfig& cfg) {
  return _create_ctxt(cfg.label, cfg.dev_idx, VK_NULL_HANDLE);
}
Context create_ctxt_windows(const ContextWindowsConfig& cfg) {
  VkSurfaceKHR surf = _create_surf_windows(cfg);
  return _create_ctxt(cfg.label, cfg.dev_idx, surf);
}
Context create_ctxt_android(const ContextAndroidConfig& cfg) {
  VkSurfaceKHR surf = _create_surf_android(cfg);
  return _create_ctxt(cfg.label, cfg.dev_idx, surf);
}
Context create_ctxt_metal(const ContextMetalConfig& cfg) {
  VkSurfaceKHR surf = _create_surf_metal(cfg);
  return _create_ctxt(cfg.label, cfg.dev_idx, surf);
}
void destroy_ctxt(Context& ctxt) {
  if (ctxt.surf != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(get_inst().inst, ctxt.surf, nullptr);
  }
  if (ctxt.dev != VK_NULL_HANDLE) {
    for (const auto& samp : ctxt.img_samplers) {
      sys::destroy_sampler(ctxt.dev, samp.second);
    }
    for (const auto& samp : ctxt.depth_img_samplers) {
      sys::destroy_sampler(ctxt.dev, samp.second);
    }
    vmaDestroyAllocator(ctxt.allocator);
    sys::destroy_dev(ctxt.dev);
    log::debug("destroyed vulkan context '", ctxt.label, "'");
  }
  for (auto pair : ctxt.desc_pool_detail.desc_set_layouts) {
    sys::destroy_desc_set_layout(ctxt.dev, pair.second);
  }
  ctxt = {};
}


} // namespace vk
} // namespace liong
