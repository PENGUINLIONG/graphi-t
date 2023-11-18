#include <set>
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

#include "sys.hpp"
#include "gft/assert.hpp"
#include "gft/util.hpp"
#include "gft/log.hpp"
#include "gft/vk/vk-context.hpp"

namespace liong {
namespace vk {

sys::SurfaceRef _create_surf_windows(const VulkanInstance &inst,
                                     const ContextWindowsConfig &cfg) {
#if VK_KHR_win32_surface
  L_ASSERT(cfg.dev_idx < inst.physdev_details.size(),
    "wanted vulkan device does not exists (#", cfg.dev_idx, " of ",
    inst.physdev_details.size(), " available devices)");
  auto physdev = inst.physdev_details.at(cfg.dev_idx);

  VkWin32SurfaceCreateInfoKHR wsci {};
  wsci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  wsci.hinstance = (HINSTANCE)cfg.hinst;
  wsci.hwnd = (HWND)cfg.hwnd;
  sys::SurfaceRef surf = sys::Surface::create(inst.inst->inst, &wsci);

  L_DEBUG("created windows surface '", cfg.label, "'");
  return surf;
#else
  panic("windows surface cannot be created on current platform");
  return VK_NULL_HANDLE;
#endif // VK_KHR_win32_surface
}

sys::SurfaceRef _create_surf_android(const VulkanInstance &inst,
                                     const ContextAndroidConfig &cfg) {
#if VK_KHR_android_surface
  L_ASSERT(cfg.dev_idx < inst.physdev_details.size(),
    "wanted vulkan device does not exists (#", cfg.dev_idx, " of ",
      inst.physdev_details.size(), " available devices)");
  auto physdev = inst.physdev_details.at(cfg.dev_idx);

  VkAndroidSurfaceCreateInfoKHR asci {};
  asci.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
  asci.window = (struct ANativeWindow* const)cfg.native_window;
  sys::SurfaceRef surf = sys::Surface::create(inst.inst->inst, &asci);

  L_DEBUG("created android surface '", cfg.label, "'");
  return surf;
#else
  panic("android surface cannot be created on current platform");
  return VK_NULL_HANDLE;
#endif // VK_KHR_android_surface
}

sys::SurfaceRef _create_surf_metal(const VulkanInstance &inst,
                                   const ContextMetalConfig &cfg) {
#if VK_EXT_metal_surface
  L_ASSERT(cfg.device_index < inst.physdev_details.size(),
    "wanted vulkan device does not exists (#", cfg.device_index, " of ",
    inst.physdev_details.size(), " available devices)");
  auto physdev = inst.physdev_details.at(cfg.device_index);

  VkMetalSurfaceCreateInfoEXT msci {};
  msci.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
  msci.pLayer = (const CAMetalLayer*)cfg.metal_layer;
  sys::SurfaceRef surf = sys::Surface::create(inst.inst->inst, &msci);

  L_DEBUG("created windows surface '", cfg.label, "'");
  return surf;
#else
  panic("metal surface cannot be created on current platform");
  return VK_NULL_HANDLE;
#endif // VK_EXT_metal_surface
}

ContextRef _create_ctxt(
  const VulkanInstance& inst,
  const std::string& label,
  uint32_t dev_idx,
  const sys::SurfaceRef& surf
) {
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
    L_WARN("context '", label, "' device does not support timestamps, "
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
      L_WARN("ignored queue family #", i, " with zero queue count");
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
    L_DEBUG("discovered queue families #", i, ": ",
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
        if (surf == nullptr) { return false; }
        VkBool32 is_supported = VK_FALSE;
        VK_ASSERT << vkGetPhysicalDeviceSurfaceSupportKHR(physdev,
          qfam_trait.qfam_idx, surf->surf, &is_supported);
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
      L_WARN("cannot find a suitable queue family for ",
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
  L_DEBUG("enabled device extensions: ", util::join(", ", dev_exts));

  sys::DeviceRef dev = sys::create_dev(physdev_detail.physdev, dqcis, dev_exts, feat);

  std::map<SubmitType, ContextSubmitDetail> submit_details;
  for (const auto& pair : queue_allocs) {
    SubmitType submit_ty = pair.first;
    uint32_t qfam_idx = pair.second;

    if (qfam_idx == VK_QUEUE_FAMILY_IGNORED) { continue; }

    ContextSubmitDetail submit_detail {};
    submit_detail.qfam_idx = qfam_idx;
    submit_detail.queue = sys::get_dev_queue(dev->dev, qfam_idx, 0);
    submit_details.insert(std::make_pair<SubmitType, ContextSubmitDetail>(
      std::move(submit_ty), std::move(submit_detail)));
  }

  std::map<ImageSampler, sys::SamplerRef> img_samplers {};
  img_samplers[L_IMAGE_SAMPLER_LINEAR] = sys::create_sampler(dev->dev,
    VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.0f, VK_COMPARE_OP_NEVER);
  img_samplers[L_IMAGE_SAMPLER_NEAREST] = sys::create_sampler(dev->dev,
    VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, 0.0f, VK_COMPARE_OP_NEVER);
  img_samplers[L_IMAGE_SAMPLER_ANISOTROPY_4] = sys::create_sampler(dev->dev,
    VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 4.0f, VK_COMPARE_OP_NEVER);

  std::map<DepthImageSampler, sys::SamplerRef> depth_img_samplers {};
  depth_img_samplers[L_DEPTH_IMAGE_SAMPLER_LINEAR] = sys::create_sampler(dev->dev,
    VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.0f, VK_COMPARE_OP_LESS);
  depth_img_samplers[L_DEPTH_IMAGE_SAMPLER_NEAREST] = sys::create_sampler(dev->dev,
    VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, 0.0f, VK_COMPARE_OP_LESS);
  depth_img_samplers[L_DEPTH_IMAGE_SAMPLER_ANISOTROPY_4] = sys::create_sampler(dev->dev,
    VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 4.0f, VK_COMPARE_OP_LESS);

  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.vulkanApiVersion = inst.api_ver;
  allocatorInfo.physicalDevice = physdev;
  allocatorInfo.device = dev->dev;
  allocatorInfo.instance = inst.inst->inst;

  sys::AllocatorRef allocator = sys::Allocator::create(&allocatorInfo);

  ContextInfo ctxt_info{};
  ctxt_info.label = label;
  ctxt_info.device_index = dev_idx;

  VulkanContextRef out = std::make_shared<VulkanContext>(inst, std::move(ctxt_info));
  out->dev = std::move(dev);
  out->surf = surf;
  out->submit_details = std::move(submit_details);
  out->img_samplers = std::move(img_samplers);
  out->depth_img_samplers = std::move(depth_img_samplers);
  out->desc_set_detail = {};
  out->cmd_pool_pool = {};
  out->query_pool_pool = {};
  out->allocator = std::move(allocator);

  L_DEBUG("created vulkan context '", label, "' on device #", dev_idx, ": ",
          inst.physdev_details.at(dev_idx).desc);

  return out;
}

ContextRef VulkanContext::create(const InstanceRef &inst,
                                 const ContextConfig &cfg) {
  VulkanInstanceRef inst_ = VulkanInstance::from_hal(inst);
  auto out = _create_ctxt(*inst_, cfg.label, cfg.device_index, VK_NULL_HANDLE);
  return out;
}
ContextRef VulkanContext::create(const InstanceRef &inst,
                                 const ContextWindowsConfig &cfg) {
  VulkanInstanceRef inst_ = VulkanInstance::from_hal(inst);
  sys::SurfaceRef surf = _create_surf_windows(*inst_, cfg);
  auto out = _create_ctxt(*inst_, cfg.label, cfg.device_index, surf);
  return out;
}
ContextRef VulkanContext::create(const InstanceRef &inst,
                                 const ContextAndroidConfig &cfg) {
  VulkanInstanceRef inst_ = VulkanInstance::from_hal(inst);
  sys::SurfaceRef surf = _create_surf_android(*inst_, cfg);
  auto out = _create_ctxt(*inst_, cfg.label, cfg.device_index, surf);
  return out;
}
ContextRef VulkanContext::create(const InstanceRef &inst,
                                 const ContextMetalConfig &cfg) {
  VulkanInstanceRef inst_ = VulkanInstance::from_hal(inst);
  sys::SurfaceRef surf = _create_surf_metal(*inst_, cfg);
  auto out = _create_ctxt(*inst_, cfg.label, cfg.device_index, surf);
  return out;
}

VulkanContext::VulkanContext(VulkanInstanceRef inst, ContextInfo &&info)
    : Context(std::move(info)), inst(inst) {
}
VulkanContext::~VulkanContext() {
  if (dev) {
    L_DEBUG("destroyed vulkan context '", info.label, "'");
  }
}


sys::DescriptorSetLayoutRef _create_desc_set_layout(
  const VulkanContext& ctxt,
  const std::vector<ResourceType>& rsc_tys
) {
  std::vector<VkDescriptorSetLayoutBinding> dslbs;
  for (auto i = 0; i < rsc_tys.size(); ++i) {
    const auto& rsc_ty = rsc_tys[i];

    VkDescriptorSetLayoutBinding dslb {};
    dslb.binding = i;
    dslb.descriptorCount = 1;
    dslb.stageFlags =
      VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT;
    switch (rsc_ty) {
    case L_RESOURCE_TYPE_UNIFORM_BUFFER:
      dslb.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      break;
    case L_RESOURCE_TYPE_STORAGE_BUFFER:
      dslb.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      break;
    case L_RESOURCE_TYPE_SAMPLED_IMAGE:
      dslb.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      break;
    case L_RESOURCE_TYPE_STORAGE_IMAGE:
      dslb.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      break;
    default:
      panic("unexpected resource type");
    }

    dslbs.emplace_back(std::move(dslb));
  }

  return sys::create_desc_set_layout(ctxt.dev->dev, dslbs);
}

sys::DescriptorSetLayoutRef VulkanContext::get_desc_set_layout(
  const std::vector<ResourceType>& rsc_tys
) {
  DescriptorSetKey desc_set_key = DescriptorSetKey::create(rsc_tys);
  auto it = desc_set_detail.desc_set_layouts.find(desc_set_key);
  if (it != desc_set_detail.desc_set_layouts.end()) {
    return it->second;
  } else {
    sys::DescriptorSetLayoutRef desc_set_layout =
      _create_desc_set_layout(*this, rsc_tys);
    desc_set_detail.desc_set_layouts[desc_set_key] = desc_set_layout;
    return desc_set_layout;
  }
}

sys::DescriptorPoolRef _create_desc_pool(
  const VulkanContext& ctxt,
  const std::vector<ResourceType>& rsc_tys
) {
  if (rsc_tys.empty()) {
    return {};
  }

  std::map<VkDescriptorType, uint32_t> desc_counter;
  for (auto i = 0; i < rsc_tys.size(); ++i) {
    const auto& rsc_ty = rsc_tys[i];

    switch (rsc_ty) {
    case L_RESOURCE_TYPE_UNIFORM_BUFFER:
      desc_counter[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] += 1;
      break;
    case L_RESOURCE_TYPE_STORAGE_BUFFER:
      desc_counter[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] += 1;
      break;
    case L_RESOURCE_TYPE_SAMPLED_IMAGE:
      desc_counter[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] += 1;
      break;
    case L_RESOURCE_TYPE_STORAGE_IMAGE:
      desc_counter[VK_DESCRIPTOR_TYPE_STORAGE_IMAGE] += 1;
      break;
    default:
      panic("unexpected resource type");
    }
  }

  // Collect `desc_pool_sizes` for checks on resource bindings.
  std::vector<VkDescriptorPoolSize> desc_pool_sizes;
  for (const auto& pair : desc_counter) {
    VkDescriptorPoolSize desc_pool_size {};
    desc_pool_size.type = pair.first;
    desc_pool_size.descriptorCount = pair.second;
    desc_pool_sizes.emplace_back(std::move(desc_pool_size));
  }

  VkDescriptorPoolCreateInfo dpci {};
  dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  dpci.poolSizeCount = static_cast<uint32_t>(desc_pool_sizes.size());
  dpci.pPoolSizes = desc_pool_sizes.data();
  dpci.maxSets = 1;

  return sys::DescriptorPool::create(ctxt.dev->dev, &dpci);
}
sys::DescriptorSetRef _alloc_desc_set(
  const VulkanContext& ctxt,
  VkDescriptorPool desc_pool,
  VkDescriptorSetLayout desc_set_layout
) {
  VkDescriptorSetAllocateInfo dsai {};
  dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dsai.descriptorPool = desc_pool;
  dsai.descriptorSetCount = 1;
  dsai.pSetLayouts = &desc_set_layout;

  return sys::DescriptorSet::create(ctxt.dev->dev, &dsai);
}

DescriptorSetKey DescriptorSetKey::create(
  const std::vector<ResourceType>& rsc_tys
) {
  std::stringstream ss;
  for (ResourceType rsc_ty : rsc_tys) {
    ss << (uint32_t)rsc_ty << ",";
  }
  return { ss.str() };
}


DescriptorSetPoolItem
VulkanContext::acquire_desc_set(const std::vector<ResourceType> &rsc_tys) {
  L_ASSERT(!rsc_tys.empty());
  DescriptorSetKey key = DescriptorSetKey::create(rsc_tys);
  if (desc_set_detail.desc_set_pool.has_free_item(key)) {
    return desc_set_detail.desc_set_pool.acquire(std::move(key));
  } else {
    sys::DescriptorPoolRef desc_pool = _create_desc_pool(*this, rsc_tys);
    sys::DescriptorSetLayoutRef desc_set_layout = get_desc_set_layout(rsc_tys);
    sys::DescriptorSetRef desc_set = _alloc_desc_set(*this, desc_pool->desc_pool, desc_set_layout->desc_set_layout);
    desc_set_detail.desc_pools.emplace_back(std::move(desc_pool));
    return desc_set_detail.desc_set_pool.create(std::move(key), std::move(desc_set));
  }
}

sys::CommandPoolRef _create_cmd_pool(const VulkanContext &ctxt,
                                     SubmitType submit_ty) {
  VkCommandPoolCreateInfo cpci {};
  cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cpci.queueFamilyIndex = ctxt.submit_details.at(submit_ty).qfam_idx;

  return sys::CommandPool::create(ctxt.dev->dev, &cpci);
}

CommandPoolPoolItem VulkanContext::acquire_cmd_pool(SubmitType submit_ty) {
  if (cmd_pool_pool.has_free_item(submit_ty)) {
    CommandPoolPoolItem item {};
    item = cmd_pool_pool.acquire(std::move(submit_ty));
    VK_ASSERT << vkResetCommandPool(*dev, *item.value(), 0);
    return item;
  } else {
    sys::CommandPoolRef cmd_pool = _create_cmd_pool(*this, submit_ty);
    return cmd_pool_pool.create(std::move(submit_ty), std::move(cmd_pool));
  }
}


sys::QueryPoolRef _create_query_pool(
  const VulkanContext& ctxt,
  VkQueryType query_ty,
  uint32_t nquery
) {
  VkQueryPoolCreateInfo qpci {};
  qpci.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  qpci.queryType = query_ty;
  qpci.queryCount = nquery;
  return sys::QueryPool::create(*ctxt.dev, &qpci);
}

QueryPoolPoolItem VulkanContext::acquire_query_pool() {
  if (query_pool_pool.has_free_item(0)) {
    return query_pool_pool.acquire(0);
  } else {
    sys::QueryPoolRef query_pool = _create_query_pool(*this, VK_QUERY_TYPE_TIMESTAMP, 2);
    return query_pool_pool.create(0, std::move(query_pool));
  }
}

} // namespace vk
} // namespace liong
