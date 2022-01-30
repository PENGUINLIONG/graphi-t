#if GFT_WITH_VULKAN

#include <map>
#include <set>
#include <algorithm>
#include "vk.hpp" // Defines `HAL_IMPL_NAMESPACE`.
#include "assert.hpp"
#include "util.hpp"
#include "log.hpp"
#include "hal/scoped-hal-impl.hpp" // `HAL_IMPL_AMESPACE` used here.

namespace liong {

namespace vk {

VkException::VkException(VkResult code) {
  switch (code) {
  case VK_ERROR_OUT_OF_HOST_MEMORY: msg = "out of host memory"; break;
  case VK_ERROR_OUT_OF_DEVICE_MEMORY: msg = "out of device memory"; break;
  case VK_ERROR_INITIALIZATION_FAILED: msg = "initialization failed"; break;
  case VK_ERROR_DEVICE_LOST: msg = "device lost"; break;
  case VK_ERROR_MEMORY_MAP_FAILED: msg = "memory map failed"; break;
  case VK_ERROR_LAYER_NOT_PRESENT: msg = "layer not supported"; break;
  case VK_ERROR_EXTENSION_NOT_PRESENT: msg = "extension may present"; break;
  case VK_ERROR_INCOMPATIBLE_DRIVER: msg = "incompatible driver"; break;
  case VK_ERROR_TOO_MANY_OBJECTS: msg = "too many objects"; break;
  case VK_ERROR_FORMAT_NOT_SUPPORTED: msg = "format not supported"; break;
  case VK_ERROR_FRAGMENTED_POOL: msg = "fragmented pool"; break;
  case VK_ERROR_OUT_OF_POOL_MEMORY: msg = "out of pool memory"; break;
  default: msg = std::string("unknown vulkan error: ") + std::to_string(code); break;
  }
}

const char* VkException::what() const noexcept { return msg.c_str(); }

#define VK_ASSERT (::liong::vk::VkAssert{})



VkInstance inst = nullptr;
std::vector<VkPhysicalDevice> physdevs {};
std::vector<std::string> physdev_descs {};



void initialize() {
  if (inst != VK_NULL_HANDLE) {
    log::warn("ignored redundant vulkan module initialization");
    return;
  }
  VkApplicationInfo app_info {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.apiVersion = VK_API_VERSION_1_0;
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

  VK_ASSERT << vkCreateInstance(&ici, nullptr, &inst);

  uint32_t nphysdev {};
  VK_ASSERT << vkEnumeratePhysicalDevices(inst, &nphysdev, nullptr);
  physdevs.resize(nphysdev);
  VK_ASSERT << vkEnumeratePhysicalDevices(inst, &nphysdev, physdevs.data());

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
  log::info("vulkan backend initialized");
}
std::string desc_dev(uint32_t idx) {
  return idx < physdev_descs.size() ? physdev_descs[idx] : std::string {};
}


// Get memory type priority based on the host access pattern. Higher the better.
uint32_t _get_mem_prior(
  MemoryAccess host_access,
  VkMemoryPropertyFlags mem_prop
) {
  if (host_access == 0) {
    if (mem_prop & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
      return 1;
    } else {
      return 0;
    }
  } else if (host_access == L_MEMORY_ACCESS_READ_BIT) {
    static const std::vector<VkMemoryPropertyFlags> PRIORITY_LUT {
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    for (int i = 0; i < PRIORITY_LUT.size(); ++i) {
      if (mem_prop == PRIORITY_LUT[i]) {
        return (uint32_t)PRIORITY_LUT.size() - i;
      }
    }
    return 0;
  } else if (host_access == L_MEMORY_ACCESS_WRITE_BIT) {
    static const std::vector<VkMemoryPropertyFlags> PRIORITY_LUT {
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
    };
    for (int i = 0; i < PRIORITY_LUT.size(); ++i) {
      if (mem_prop == PRIORITY_LUT[i]) {
        return (uint32_t)PRIORITY_LUT.size() - i;
      }
    }
    return 0;
  } else {
    static const std::vector<VkMemoryPropertyFlags> PRIORITY_LUT {
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
    };
    for (int i = 0; i < PRIORITY_LUT.size(); ++i) {
      if (mem_prop == PRIORITY_LUT[i]) {
        return (uint32_t)PRIORITY_LUT.size() - i;
      }
    }
    return 0;
  }
}
Context create_ctxt(const ContextConfig& cfg) {
  if (inst == VK_NULL_HANDLE) {
    initialize();
  }
  assert(cfg.dev_idx < physdevs.size(),
    "wanted vulkan device does not exists (#", cfg.dev_idx, " of ",
      physdevs.size(), " available devices)");
  auto physdev = physdevs[cfg.dev_idx];

  VkPhysicalDeviceFeatures feat;
  vkGetPhysicalDeviceFeatures(physdev, &feat);

  VkPhysicalDeviceProperties physdev_prop;
  vkGetPhysicalDeviceProperties(physdev, &physdev_prop);

  if (physdev_prop.limits.timestampComputeAndGraphics == VK_FALSE) {
    log::warn("context '", cfg.label, "' device does not support timestamps, "
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
  assert(!qfam_props.empty(), "cannot find any queue family on device #",
    cfg.dev_idx);

  struct SubmitTypeQueueRequirement {
    SubmitType submit_ty;
    VkQueueFlags queue_flags;
    const char* submit_ty_name;
    std::vector<const char*> cmd_names;
  };
  std::vector<SubmitTypeQueueRequirement> submit_ty_reqs {
    SubmitTypeQueueRequirement {
      L_SUBMIT_TYPE_GRAPHICS,
      VK_QUEUE_GRAPHICS_BIT,
      "GRAPHICS",
      {
        "DRAW",
        "DRAW_INDEXED",
      },
    },
    SubmitTypeQueueRequirement {
      L_SUBMIT_TYPE_COMPUTE,
      VK_QUEUE_COMPUTE_BIT,
      "COMPUTE",
      {
        "DISPATCH",
      },
    },
  };

  std::map<uint32_t, uint32_t> queue_allocs;
  for (size_t i = 0; i < submit_ty_reqs.size(); ++i) {
    const SubmitTypeQueueRequirement& submit_ty_queue_req = submit_ty_reqs[i];
    VkQueueFlags req_queue_flags = submit_ty_queue_req.queue_flags;

    queue_allocs[submit_ty_queue_req.submit_ty] = VK_QUEUE_FAMILY_IGNORED;
    uint32_t& qfam_idx_alloc = queue_allocs[submit_ty_queue_req.submit_ty];
    // Search from a combination of queue traits to queues of a single trait.
    for (auto it = qfam_map.rbegin(); it != qfam_map.rend(); ++it) {
      // We have already found a suitable queue family; no need to continue. 
      if (qfam_idx_alloc != VK_QUEUE_FAMILY_IGNORED) { break; }

      // Otherwise, look for a queue family that could satisfy all the required
      // flags.
      for (auto qfam_trait : it->second) {
        if ((req_queue_flags & qfam_trait.queue_flags) == req_queue_flags) {
          qfam_idx_alloc = qfam_trait.qfam_idx;
          break;
        }
      }
    }

    if (qfam_idx_alloc == VK_QUEUE_FAMILY_IGNORED) {
      log::warn("cannot find a suitable queue family for ",
        submit_ty_queue_req.submit_ty_name, ", the following commands won't be "
        "available: ", util::join(", ", submit_ty_queue_req.cmd_names));
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
  log::debug("enabled device extensions: ",
    util::join(", ", dev_ext_names));

  VkDeviceCreateInfo dci {};
  dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  dci.pEnabledFeatures = &feat;
  dci.queueCreateInfoCount = static_cast<uint32_t>(dqcis.size());
  dci.pQueueCreateInfos = dqcis.data();
  dci.enabledExtensionCount = (uint32_t)dev_ext_names.size();
  dci.ppEnabledExtensionNames = dev_ext_names.data();

  VkDevice dev;
  VK_ASSERT << vkCreateDevice(physdev, &dci, nullptr, &dev);

  std::vector<ContextSubmitDetail> submit_details;
  for (const auto& pair : queue_allocs) {
    uint32_t qfam_idx = pair.second;
    ContextSubmitDetail submit_detail {};
    submit_detail.qfam_idx = qfam_idx;
    vkGetDeviceQueue(dev, qfam_idx, 0, &submit_detail.queue);
    submit_details.emplace_back(std::move(submit_detail));
  }

  VkPhysicalDeviceMemoryProperties mem_prop;
  vkGetPhysicalDeviceMemoryProperties(physdev, &mem_prop);

  // Priority -> memory type.
  std::array<std::vector<uint32_t>, 4> mem_ty_idxs_by_host_access {
    util::arrange(mem_prop.memoryTypeCount),
    util::arrange(mem_prop.memoryTypeCount),
    util::arrange(mem_prop.memoryTypeCount),
    util::arrange(mem_prop.memoryTypeCount),
  };
  for (int host_access = 0; host_access < 4; ++host_access) {
    auto& mem_ty_idxs = mem_ty_idxs_by_host_access[host_access];
    std::vector<uint32_t> priors = util::map<uint32_t, uint32_t>(
      mem_ty_idxs,
      [&](const uint32_t& i) -> uint32_t {
        const auto& mem_ty = mem_prop.memoryTypes[i];
        return _get_mem_prior(host_access, mem_ty.propertyFlags);
      });
    std::sort(mem_ty_idxs.begin(), mem_ty_idxs.end(),
      [&](uint32_t ilhs, uint32_t irhs) {
        return priors[ilhs] > priors[irhs];
      });
  }

  VkSamplerCreateInfo sci {};
  sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sci.magFilter = VK_FILTER_LINEAR;
  sci.minFilter = VK_FILTER_LINEAR;
  sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sci.unnormalizedCoordinates = VK_FALSE;

  VkSampler fast_samp;
  VK_ASSERT << vkCreateSampler(dev, &sci, nullptr, &fast_samp);

  log::debug("created vulkan context '", cfg.label, "' on device #",
    cfg.dev_idx, ": ", physdev_descs[cfg.dev_idx]);
  return Context {
    dev, physdev, std::move(physdev_prop), std::move(submit_details),
    std::move(queue_allocs), std::move(mem_ty_idxs_by_host_access),
    fast_samp, cfg
  };
}
void destroy_ctxt(Context& ctxt) {
  if (ctxt.dev != VK_NULL_HANDLE) {
    vkDestroySampler(ctxt.dev, ctxt.fast_samp, nullptr);
    vkDestroyDevice(ctxt.dev, nullptr);
    log::debug("destroyed vulkan context '", ctxt.ctxt_cfg.label, "'");
  }
  ctxt = {};
}
const ContextConfig& get_ctxt_cfg(const Context& ctxt) {
  return ctxt.ctxt_cfg;
}



Buffer create_buf(const Context& ctxt, const BufferConfig& buf_cfg) {
  VkBufferCreateInfo bci {};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (buf_cfg.usage & L_BUFFER_USAGE_STAGING_BIT) {
    bci.usage |=
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  if (buf_cfg.usage & L_BUFFER_USAGE_UNIFORM_BIT) {
    bci.usage |=
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  if (buf_cfg.usage & L_BUFFER_USAGE_STORAGE_BIT) {
    bci.usage =
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  if (buf_cfg.usage & L_BUFFER_USAGE_VERTEX_BIT) {
    bci.usage =
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  if (buf_cfg.usage & L_BUFFER_USAGE_INDEX_BIT) {
    bci.usage =
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  bci.size = buf_cfg.size;

  VkBuffer buf;
  VK_ASSERT << vkCreateBuffer(ctxt.dev, &bci, nullptr, &buf);

  VkMemoryRequirements mr {};
  vkGetBufferMemoryRequirements(ctxt.dev, buf, &mr);

  VkMemoryAllocateInfo mai {};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.allocationSize = mr.size;
  mai.memoryTypeIndex = 0xFF;
  for (auto mem_ty_idx : ctxt.mem_ty_idxs_by_host_access[buf_cfg.host_access]) {
    if (((1 << mem_ty_idx) & mr.memoryTypeBits) != 0) {
      mai.memoryTypeIndex = mem_ty_idx;
      break;
    }
  }
  if (mai.memoryTypeIndex == 0xFF) {
    panic("host access pattern cannot be satisfied");
  }

  VkDeviceMemory devmem;
  VK_ASSERT << vkAllocateMemory(ctxt.dev, &mai, nullptr, &devmem);

  VK_ASSERT << vkBindBufferMemory(ctxt.dev, buf, devmem, 0);

  log::debug("created buffer '", buf_cfg.label, "'");
  return Buffer { &ctxt, devmem, buf, buf_cfg };
}
void destroy_buf(Buffer& buf) {
  if (buf.buf != VK_NULL_HANDLE) {
    vkDestroyBuffer(buf.ctxt->dev, buf.buf, nullptr);
    vkFreeMemory(buf.ctxt->dev, buf.devmem, nullptr);
    log::debug("destroyed buffer '", buf.buf_cfg.label, "'");
    buf = {};
  }
}
const BufferConfig& get_buf_cfg(const Buffer& buf) {
  return buf.buf_cfg;
}



void map_buf_mem(
  const BufferView& buf,
  MemoryAccess map_access,
  void*& mapped
) {
  VK_ASSERT << vkMapMemory(buf.buf->ctxt->dev, buf.buf->devmem, buf.offset,
    buf.size, 0, &mapped);
  log::debug("mapped buffer '", buf.buf->buf_cfg.label, "' from ", buf.offset,
    " to ", buf.offset + buf.size);
}
void unmap_buf_mem(
  const BufferView& buf,
  void* mapped
) {
  vkUnmapMemory(buf.buf->ctxt->dev, buf.buf->devmem);
  log::debug("unmapped buffer '", buf.buf->buf_cfg.label, "'");
}



VkFormat _make_img_fmt(PixelFormat fmt) {
  if (fmt.is_half) {
    panic("half-precision texture not supported");
  } else if (fmt.is_single) {
    switch (fmt.get_ncomp()) {
    case 1: return VK_FORMAT_R32_SFLOAT;
    case 2: return VK_FORMAT_R32G32_SFLOAT;
    case 3: return VK_FORMAT_R32G32B32_SFLOAT;
    case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
    }
  } else if (fmt.is_signed) {
    switch (fmt.int_exp2) {
    case 1:
      switch (fmt.get_ncomp()) {
      case 1: return VK_FORMAT_R8_SNORM;
      case 2: return VK_FORMAT_R8G8_SNORM;
      case 3: return VK_FORMAT_R8G8B8_SNORM;
      case 4: return VK_FORMAT_R8G8B8A8_SNORM;
      }
    case 2:
      switch (fmt.int_exp2) {
      case 1: return VK_FORMAT_R16_SINT;
      case 2: return VK_FORMAT_R16G16_SINT;
      case 3: return VK_FORMAT_R16G16B16_SINT;
      case 4: return VK_FORMAT_R16G16B16A16_SINT;
      }
    case 3:
      switch (fmt.int_exp2) {
      case 1: return VK_FORMAT_R32_SINT;
      case 2: return VK_FORMAT_R32G32_SINT;
      case 3: return VK_FORMAT_R32G32B32_SINT;
      case 4: return VK_FORMAT_R32G32B32A32_SINT;
      }
    }
  } else {
    switch (fmt.int_exp2) {
    case 1:
      switch (fmt.get_ncomp()) {
      case 1: return VK_FORMAT_R8_UNORM;
      case 2: return VK_FORMAT_R8G8_UNORM;
      case 3: return VK_FORMAT_R8G8B8_UNORM;
      case 4: return VK_FORMAT_R8G8B8A8_UNORM;
      }
    case 2:
      switch (fmt.int_exp2) {
      case 1: return VK_FORMAT_R16_UINT;
      case 2: return VK_FORMAT_R16G16_UINT;
      case 3: return VK_FORMAT_R16G16B16_UINT;
      case 4: return VK_FORMAT_R16G16B16A16_UINT;
      }
    case 3:
      switch (fmt.int_exp2) {
      case 1: return VK_FORMAT_R32_UINT;
      case 2: return VK_FORMAT_R32G32_UINT;
      case 3: return VK_FORMAT_R32G32B32_UINT;
      case 4: return VK_FORMAT_R32G32B32A32_UINT;
      }
    }
  }
  panic("unrecognized pixel format");
  return VK_FORMAT_UNDEFINED;
}
Image create_img(const Context& ctxt, const ImageConfig& img_cfg) {
  VkFormat fmt = _make_img_fmt(img_cfg.fmt);
  VkImageUsageFlags usage = 0;
  SubmitType init_submit_ty = L_SUBMIT_TYPE_ANY;
  bool is_staging_img = false;

  if (img_cfg.usage & L_IMAGE_USAGE_SAMPLED_BIT) {
    usage |=
      VK_IMAGE_USAGE_SAMPLED_BIT |
      VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    init_submit_ty = L_SUBMIT_TYPE_ANY;
  }
  if (img_cfg.usage & L_IMAGE_USAGE_STORAGE_BIT) {
    usage |=
      VK_IMAGE_USAGE_STORAGE_BIT |
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
      VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    init_submit_ty = L_SUBMIT_TYPE_ANY;
  }
  // KEEP THIS AFTER DESCRIPTOR RESOURCE USAGES.
  if (img_cfg.usage & L_IMAGE_USAGE_ATTACHMENT_BIT) {
    usage |=
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
      VK_IMAGE_USAGE_TRANSFER_DST_BIT |
      VK_IMAGE_USAGE_SAMPLED_BIT |
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
      VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    init_submit_ty = L_SUBMIT_TYPE_GRAPHICS;
  }
  // KEEP THIS AT THE END.
  if (img_cfg.usage & L_IMAGE_USAGE_STAGING_BIT) {
    assert((img_cfg.usage & (~L_IMAGE_USAGE_STAGING_BIT)) == 0, "staging image "
      "can only be used for transfer");
    usage |=
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
      VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    init_submit_ty = L_SUBMIT_TYPE_ANY;
    // The only one case that we can feed the image with our data directly by
    // memory mapping.
    is_staging_img = true;
  }

  // Check whether the device support our use case.
  VkImageFormatProperties ifp;
  VK_ASSERT << vkGetPhysicalDeviceImageFormatProperties(ctxt.physdev, fmt,
    VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &ifp);

  VkImageLayout layout = is_staging_img ?
    VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;

  VkImageCreateInfo ici {};
  ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ici.imageType = VK_IMAGE_TYPE_2D;
  ici.format = fmt;
  ici.extent.width = img_cfg.width;
  ici.extent.height = img_cfg.height;
  ici.extent.depth = 1;
  ici.mipLevels = 1;
  ici.arrayLayers = 1;
  ici.samples = VK_SAMPLE_COUNT_1_BIT;
  ici.tiling = is_staging_img ?
    VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
  ici.usage = usage;
  ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  ici.initialLayout = layout;

  VkImage img;
  VK_ASSERT << vkCreateImage(ctxt.dev, &ici, nullptr, &img);

  VkMemoryRequirements mr {};
  vkGetImageMemoryRequirements(ctxt.dev, img, &mr);

  VkMemoryAllocateInfo mai {};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.allocationSize = mr.size;
  mai.memoryTypeIndex = 0xFF;
  for (auto mem_ty_idx : ctxt.mem_ty_idxs_by_host_access[img_cfg.host_access]) {
    if (((1 << mem_ty_idx) & mr.memoryTypeBits) != 0) {
      mai.memoryTypeIndex = mem_ty_idx;
      break;
    }
  }
  if (mai.memoryTypeIndex == 0xFF) {
    panic("host access pattern cannot be satisfied");
  }

  VkDeviceMemory devmem;
  VK_ASSERT << vkAllocateMemory(ctxt.dev, &mai, nullptr, &devmem);

  VK_ASSERT << vkBindImageMemory(ctxt.dev, img, devmem, 0);

  VkImageView img_view = VK_NULL_HANDLE;
  if (!is_staging_img) {
    VkImageViewCreateInfo ivci {};
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image = img;
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = fmt;
    ivci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    ivci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    ivci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    ivci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ivci.subresourceRange.baseArrayLayer = 0;
    ivci.subresourceRange.layerCount = 1;
    ivci.subresourceRange.baseMipLevel = 0;
    ivci.subresourceRange.levelCount = 1;

    VK_ASSERT << vkCreateImageView(ctxt.dev, &ivci, nullptr, &img_view);
  }

  log::debug("created image '", img_cfg.label, "'");
  uint32_t qfam_idx = ctxt.get_submit_ty_qfam_idx(init_submit_ty);
  return Image {
    &ctxt, devmem, img, img_view, img_cfg, is_staging_img
  };
}
void destroy_img(Image& img) {
  if (img.img != VK_NULL_HANDLE) {
    vkDestroyImageView(img.ctxt->dev, img.img_view, nullptr);
    vkDestroyImage(img.ctxt->dev, img.img, nullptr);
    vkFreeMemory(img.ctxt->dev, img.devmem, nullptr);

    log::debug("destroyed image '", img.img_cfg.label, "'");
    img = {};
  }
}
const ImageConfig& get_img_cfg(const Image& img) {
  return img.img_cfg;
}



VkFormat _make_depth_fmt(DepthFormat fmt) {
  if (fmt.nbit_depth == 16 && fmt.nbit_stencil == 0) {
    return VK_FORMAT_D16_UNORM;
  }
  if (fmt.nbit_depth == 24 && fmt.nbit_stencil == 0) {
    return VK_FORMAT_X8_D24_UNORM_PACK32;
  }
  if (fmt.nbit_depth == 32 && fmt.nbit_stencil == 0) {
    return VK_FORMAT_D32_SFLOAT;
  }
  if (fmt.nbit_depth == 0 && fmt.nbit_stencil == 8) {
    return VK_FORMAT_S8_UINT;
  }
  if (fmt.nbit_depth == 16 && fmt.nbit_stencil == 8) {
    return VK_FORMAT_D16_UNORM_S8_UINT;
  }
  if (fmt.nbit_depth == 24 && fmt.nbit_stencil == 8) {
    return VK_FORMAT_D24_UNORM_S8_UINT;
  }
  if (fmt.nbit_depth == 32 && fmt.nbit_stencil == 8) {
    return VK_FORMAT_D32_SFLOAT_S8_UINT;
  }
  panic("unsupported depth format");
  return VK_FORMAT_UNDEFINED;
}

DepthImage create_depth_img(
  const Context& ctxt,
  const DepthImageConfig& depth_img_cfg
) {
  VkFormat fmt = _make_depth_fmt(depth_img_cfg.fmt);
  VkImageUsageFlags usage =
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
    VK_IMAGE_USAGE_SAMPLED_BIT;

  // Check whether the device support our use case.
  VkImageFormatProperties ifp;
  VK_ASSERT << vkGetPhysicalDeviceImageFormatProperties(
    physdevs[ctxt.ctxt_cfg.dev_idx], fmt, VK_IMAGE_TYPE_2D,
    VK_IMAGE_TILING_OPTIMAL, usage, 0, &ifp);

  VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkImageCreateInfo ici {};
  ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ici.imageType = VK_IMAGE_TYPE_2D;
  ici.format = fmt;
  ici.extent.width = (uint32_t)depth_img_cfg.width;
  ici.extent.height = (uint32_t)depth_img_cfg.height;
  ici.extent.depth = 1;
  ici.mipLevels = 1;
  ici.arrayLayers = 1;
  ici.samples = VK_SAMPLE_COUNT_1_BIT;
  ici.tiling = VK_IMAGE_TILING_OPTIMAL;
  ici.usage = usage;
  ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  ici.initialLayout = layout;

  VkImage img;
  VK_ASSERT << vkCreateImage(ctxt.dev, &ici, nullptr, &img);

  VkMemoryRequirements mr {};
  vkGetImageMemoryRequirements(ctxt.dev, img, &mr);

  VkMemoryAllocateInfo mai {};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.allocationSize = mr.size;
  mai.memoryTypeIndex = 0xFF;
  auto host_access = 0;
  for (auto mem_ty_idx : ctxt.mem_ty_idxs_by_host_access[host_access]) {
    if (((1 << mem_ty_idx) & mr.memoryTypeBits) != 0) {
      mai.memoryTypeIndex = mem_ty_idx;
      break;
    }
  }
  if (mai.memoryTypeIndex == 0xFF) {
    panic("depth image has no host access but it cannot be satisfied");
  }

  VkDeviceMemory devmem;
  VK_ASSERT << vkAllocateMemory(ctxt.dev, &mai, nullptr, &devmem);

  VK_ASSERT << vkBindImageMemory(ctxt.dev, img, devmem, 0);

  VkImageViewCreateInfo ivci {};
  ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ivci.image = img;
  ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
  ivci.format = fmt;
  ivci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  ivci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  ivci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  ivci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  ivci.subresourceRange.aspectMask =
    (depth_img_cfg.fmt.nbit_depth > 0 ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
    (depth_img_cfg.fmt.nbit_stencil > 0 ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
  ivci.subresourceRange.baseArrayLayer = 0;
  ivci.subresourceRange.layerCount = 1;
  ivci.subresourceRange.baseMipLevel = 0;
  ivci.subresourceRange.levelCount = 1;

  VkImageView img_view;
  VK_ASSERT << vkCreateImageView(ctxt.dev, &ivci, nullptr, &img_view);

  log::info("created depth image '", depth_img_cfg.label, "'");
  return DepthImage {
    &ctxt, devmem, (size_t)mr.size, img, img_view, depth_img_cfg
  };
}
void destroy_depth_img(DepthImage& depth_img) {
  if (depth_img.img) {
    vkDestroyImageView(depth_img.ctxt->dev, depth_img.img_view, nullptr);
    vkDestroyImage(depth_img.ctxt->dev, depth_img.img, nullptr);
    vkFreeMemory(depth_img.ctxt->dev, depth_img.devmem, nullptr);

    log::info("destroyed depth image '", depth_img.depth_img_cfg.label, "'");
    depth_img = {};
  }
}
const DepthImageConfig& get_depth_img_cfg(const DepthImage& depth_img) {
  return depth_img.depth_img_cfg;
}



void map_img_mem(
  const ImageView& img,
  MemoryAccess map_access,
  void*& mapped,
  size_t& row_pitch
) {
  VkImageSubresource is {};
  is.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  is.arrayLayer = 0;
  is.mipLevel = 0;

  VkSubresourceLayout sl {};
  vkGetImageSubresourceLayout(img.img->ctxt->dev, img.img->img, &is, &sl);
  size_t offset = sl.offset;
  size_t size = sl.size;

  VK_ASSERT << vkMapMemory(img.img->ctxt->dev, img.img->devmem, sl.offset,
    sl.size, 0, &mapped);
  row_pitch = sl.rowPitch;

  log::debug("mapped image '", img.img->img_cfg.label, "' from (", img.x_offset,
    ", ", img.y_offset, ") to (", img.x_offset + img.width, ", ",
    img.y_offset + img.height, ")");
}
void unmap_img_mem(
  const ImageView& img,
  void* mapped
) {
  vkUnmapMemory(img.img->ctxt->dev, img.img->devmem);
  log::debug("unmapped image '", img.img->img_cfg.label, "'");
}




VkDescriptorSetLayout _create_desc_set_layout(
  const Context& ctxt,
  const std::vector<ResourceType> rsc_tys,
  std::vector<VkDescriptorPoolSize>& desc_pool_sizes
) {
  std::vector<VkDescriptorSetLayoutBinding> dslbs;
  std::map<VkDescriptorType, uint32_t> desc_counter;
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
      dslb.pImmutableSamplers = &ctxt.fast_samp;
      break;
    case L_RESOURCE_TYPE_STORAGE_IMAGE:
      dslb.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      break;
    default:
      panic("unexpected resource type");
    }
    desc_counter[dslb.descriptorType] += 1;

    dslbs.emplace_back(std::move(dslb));

    // Collect `desc_pool_sizes` for checks on resource bindings.
    for (const auto& pair : desc_counter) {
      VkDescriptorPoolSize desc_pool_size;
      desc_pool_size.type = pair.first;
      desc_pool_size.descriptorCount = pair.second;
      desc_pool_sizes.emplace_back(std::move(desc_pool_size));
    }
  }

  VkDescriptorSetLayoutCreateInfo dslci {};
  dslci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  dslci.bindingCount = (uint32_t)dslbs.size();
  dslci.pBindings = dslbs.data();

  VkDescriptorSetLayout desc_set_layout;
  VK_ASSERT <<
    vkCreateDescriptorSetLayout(ctxt.dev, &dslci, nullptr, &desc_set_layout);

  return desc_set_layout;
}
VkPipelineLayout _create_pipe_layout(
  const Context& ctxt,
  VkDescriptorSetLayout desc_set_layout
) {
  VkPipelineLayoutCreateInfo plci {};
  plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  plci.setLayoutCount = 1;
  plci.pSetLayouts = &desc_set_layout;

  VkPipelineLayout pipe_layout;
  VK_ASSERT << vkCreatePipelineLayout(ctxt.dev, &plci, nullptr, &pipe_layout);

  return pipe_layout;
}
VkShaderModule _create_shader_mod(
  const Context& ctxt,
  const void* code,
  size_t code_size
) {
  VkShaderModuleCreateInfo smci {};
  smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  smci.pCode = (const uint32_t*)code;
  smci.codeSize = code_size;

  VkShaderModule shader_mod;
  VK_ASSERT << vkCreateShaderModule(ctxt.dev, &smci, nullptr, &shader_mod);

  return shader_mod;
}
Task create_comp_task(
  const Context& ctxt,
  const ComputeTaskConfig& cfg
) {
  std::vector<VkDescriptorPoolSize> desc_pool_sizes;
  VkDescriptorSetLayout desc_set_layout = _create_desc_set_layout(ctxt,
    cfg.rsc_tys, desc_pool_sizes);
  VkPipelineLayout pipe_layout = _create_pipe_layout(ctxt, desc_set_layout);
  VkShaderModule shader_mod = _create_shader_mod(ctxt, cfg.code, cfg.code_size);

  // Specialize to set local group size.
  VkSpecializationMapEntry spec_map_entries[] = {
    VkSpecializationMapEntry { 0, 0 * sizeof(int32_t), sizeof(int32_t) },
    VkSpecializationMapEntry { 1, 1 * sizeof(int32_t), sizeof(int32_t) },
    VkSpecializationMapEntry { 2, 2 * sizeof(int32_t), sizeof(int32_t) },
  };
  VkSpecializationInfo spec_info {};
  spec_info.pData = &cfg.workgrp_size;
  spec_info.dataSize = sizeof(cfg.workgrp_size);
  spec_info.mapEntryCount = 3;
  spec_info.pMapEntries = spec_map_entries;

  VkPipelineShaderStageCreateInfo pssci {};
  pssci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pssci.pName = cfg.entry_name.c_str();
  pssci.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  pssci.module = shader_mod;
  pssci.pSpecializationInfo = &spec_info;

  VkComputePipelineCreateInfo cpci {};
  cpci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  cpci.stage = std::move(pssci);
  cpci.layout = pipe_layout;

  VkPipeline pipe;
  VK_ASSERT << vkCreateComputePipelines(ctxt.dev, VK_NULL_HANDLE, 1, &cpci,
    nullptr, &pipe);

  log::debug("created compute task '", cfg.label, "'");
  return Task {
    &ctxt, nullptr, desc_set_layout, pipe_layout, pipe, cfg.rsc_tys,
    { shader_mod }, std::move(desc_pool_sizes), cfg.label };
}
VkAttachmentLoadOp _get_load_op(AttachmentAccess attm_access) {
  if (attm_access & L_ATTACHMENT_ACCESS_CLEAR) {
    return VK_ATTACHMENT_LOAD_OP_CLEAR;
  }
  if (attm_access & L_ATTACHMENT_ACCESS_LOAD) {
    return VK_ATTACHMENT_LOAD_OP_LOAD;
  }
  return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}
VkAttachmentStoreOp _get_store_op(AttachmentAccess attm_access) {
  if (attm_access & L_ATTACHMENT_ACCESS_STORE) {
    return VK_ATTACHMENT_STORE_OP_STORE;
  }
  return VK_ATTACHMENT_STORE_OP_DONT_CARE;
}
VkRenderPass _create_pass(
  const Context& ctxt,
  const std::vector<AttachmentConfig>& attm_cfgs
) {
  struct SubpassAttachmentReference {
    std::vector<VkAttachmentReference> color_attm_ref;
    bool has_depth_attm;
    VkAttachmentReference depth_attm_ref;
  };

  SubpassAttachmentReference sar {};
  std::vector<VkAttachmentDescription> ads;
  for (uint32_t i = 0; i < attm_cfgs.size(); ++i) {
    const AttachmentConfig& attm_cfg = attm_cfgs.at(i);
    uint32_t iattm = i;

    VkAttachmentReference ar {};
    ar.attachment = i;
    VkAttachmentDescription ad {};
    ad.samples = VK_SAMPLE_COUNT_1_BIT;
    ad.loadOp = _get_load_op(attm_cfg.attm_access);
    ad.storeOp = _get_store_op(attm_cfg.attm_access);
    switch (attm_cfg.attm_ty) {
    case L_ATTACHMENT_TYPE_COLOR:
    {
      const Image& color_img = *attm_cfg.color_img;
      ar.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      ad.format = _make_img_fmt(color_img.img_cfg.fmt);
      ad.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      ad.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
      sar.color_attm_ref.emplace_back(std::move(ar));
      break;
    }
    case L_ATTACHMENT_TYPE_DEPTH:
    {
      assert(!sar.has_depth_attm, "subpass can only have one depth attachment");
      const DepthImage& depth_img = *attm_cfg.depth_img;
      ar.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      ad.format = _make_depth_fmt(depth_img.depth_img_cfg.fmt);
      ad.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      ad.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
      sar.has_depth_attm = true;
      sar.depth_attm_ref = std::move(ar);
      break;
    }
    default: panic();
    }

    ads.emplace_back(std::move(ad));
  }

  // TODO: (penguinliong) Support input attachments.
  std::vector<VkSubpassDescription> sds;
  {
    VkSubpassDescription sd {};
    sd.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sd.inputAttachmentCount = 0;
    sd.pInputAttachments = nullptr;
    sd.colorAttachmentCount = (uint32_t)sar.color_attm_ref.size();
    sd.pColorAttachments = sar.color_attm_ref.data();
    sd.pResolveAttachments = nullptr;
    sd.pDepthStencilAttachment =
      sar.has_depth_attm ? &sar.depth_attm_ref : nullptr;
    sd.preserveAttachmentCount = 0;
    sd.pPreserveAttachments = nullptr;
    sds.emplace_back(std::move(sd));
  }
  VkRenderPassCreateInfo rpci {};
  rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  rpci.attachmentCount = (uint32_t)ads.size();
  rpci.pAttachments = ads.data();
  rpci.subpassCount = (uint32_t)sds.size();
  rpci.pSubpasses = sds.data();
  // TODO: (penguinliong) Implement subpass dependency resolution in the future.
  rpci.dependencyCount = 0;
  rpci.pDependencies = nullptr;

  VkRenderPass pass;
  VK_ASSERT << vkCreateRenderPass(ctxt.dev, &rpci, nullptr, &pass);

  return pass;
}
VkFramebuffer _create_framebuf(
  const Context& ctxt,
  VkRenderPass pass,
  const std::vector<AttachmentConfig>& attm_cfgs,
  uint32_t width,
  uint32_t height
) {
  std::vector<VkImageView> attm_img_views;
  for (const AttachmentConfig& attm_cfg : attm_cfgs) {
    switch (attm_cfg.attm_ty) {
    case L_ATTACHMENT_TYPE_COLOR:
      assert(attm_cfg.color_img->img_cfg.width == width &&
        attm_cfg.color_img->img_cfg.height == height,
        "color attachment size mismatches framebuffer size");
      attm_img_views.emplace_back(attm_cfg.color_img->img_view);
      break;
    case L_ATTACHMENT_TYPE_DEPTH:
      assert(attm_cfg.depth_img->depth_img_cfg.width == width &&
        attm_cfg.depth_img->depth_img_cfg.height == height,
        "depth attachment size mismatches framebuffer size");
      attm_img_views.emplace_back(attm_cfg.depth_img->img_view);
      break;
    default: panic("unexpected attachment type");
    }
  }

  VkFramebufferCreateInfo fci {};
  fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fci.renderPass = pass;
  fci.attachmentCount = (uint32_t)attm_img_views.size();
  fci.pAttachments = attm_img_views.data();
  fci.width = width;
  fci.height = height;
  fci.layers = 1;

  VkFramebuffer framebuf;
  VK_ASSERT << vkCreateFramebuffer(ctxt.dev, &fci, nullptr, &framebuf);

  return framebuf;
}

RenderPass create_pass(const Context& ctxt, const RenderPassConfig& cfg) {
  VkRenderPass pass = _create_pass(ctxt, cfg.attm_cfgs);
  VkFramebuffer framebuf = _create_framebuf(ctxt, pass, cfg.attm_cfgs,
    cfg.width, cfg.height);

  VkRect2D viewport {};
  viewport.extent.width = cfg.width;
  viewport.extent.height = cfg.height;

  std::vector<VkClearValue> clear_values {};
  clear_values.resize(cfg.attm_cfgs.size());
  for (size_t i = 0; i < cfg.attm_cfgs.size(); ++i) {
    auto& clear_value = clear_values[i];
    switch (cfg.attm_cfgs[i].attm_ty) {
    case L_ATTACHMENT_TYPE_COLOR:
      clear_value.color.float32[0] = 0.0f;
      clear_value.color.float32[0] = 0.0f;
      clear_value.color.float32[0] = 0.0f;
      clear_value.color.float32[0] = 0.0f;
      break;
    case L_ATTACHMENT_TYPE_DEPTH:
      clear_value.depthStencil.depth = 1.0f;
      clear_value.depthStencil.stencil = 0;
      break;
    default: panic();
    }
  }

  log::info("created render pass '", cfg.label, "'");
  return RenderPass {
    &ctxt, std::move(viewport), pass, framebuf, cfg, clear_values
  };
}
void destroy_pass(RenderPass& pass) {
  vkDestroyFramebuffer(pass.ctxt->dev, pass.framebuf, nullptr);
  vkDestroyRenderPass(pass.ctxt->dev, pass.pass, nullptr);
  log::info("destroyed render pass '", pass.pass_cfg.label, "'");
}



Task create_graph_task(
  const RenderPass& pass,
  const GraphicsTaskConfig& cfg
) {
  const Context& ctxt = *pass.ctxt;

  std::vector<VkDescriptorPoolSize> desc_pool_sizes;
  VkDescriptorSetLayout desc_set_layout =
    _create_desc_set_layout(ctxt, cfg.rsc_tys, desc_pool_sizes);
  VkPipelineLayout pipe_layout = _create_pipe_layout(ctxt, desc_set_layout);
  VkShaderModule vert_shader_mod =
    _create_shader_mod(ctxt, cfg.vert_code, cfg.vert_code_size);
  VkShaderModule frag_shader_mod =
    _create_shader_mod(ctxt, cfg.frag_code, cfg.frag_code_size);

  VkPipelineShaderStageCreateInfo psscis[2] {};
  {
    VkPipelineShaderStageCreateInfo& pssci = psscis[0];
    pssci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pssci.pName = cfg.vert_entry_name.c_str();
    pssci.stage = VK_SHADER_STAGE_VERTEX_BIT;
    pssci.module = vert_shader_mod;
  }
  {
    VkPipelineShaderStageCreateInfo& pssci = psscis[1];
    pssci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pssci.pName = cfg.frag_entry_name.c_str();
    pssci.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pssci.module = frag_shader_mod;
  }

 VkVertexInputBindingDescription vibd {};
  std::vector<VkVertexInputAttributeDescription> viads;
  size_t base_offset = 0;
  for (auto i = 0; i < cfg.vert_inputs.size(); ++i) {
    auto& vert_input = cfg.vert_inputs[i];
    size_t fmt_size = vert_input.fmt.get_fmt_size();

    vibd.binding = 0;
    vibd.stride = 0; // Will be assigned later.
    switch (cfg.vert_inputs[i].rate) {
    case L_VERTEX_INPUT_RATE_VERTEX:
      vibd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      break;
    case L_VERTEX_INPUT_RATE_INSTANCE:
      vibd.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
      panic("instanced draw is currently unsupported");
      break;
    default:
      panic("unexpected vertex input rate");
    }

    VkVertexInputAttributeDescription viad {};
    viad.location = i;
    viad.binding = 0;
    viad.format = _make_img_fmt(vert_input.fmt);
    viad.offset = (uint32_t)base_offset;
    viads.emplace_back(std::move(viad));

    base_offset += fmt_size;
  }
  vibd.stride = (uint32_t)base_offset;

  VkPipelineVertexInputStateCreateInfo pvisci {};
  pvisci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  pvisci.vertexBindingDescriptionCount = 1;
  pvisci.pVertexBindingDescriptions = &vibd;
  pvisci.vertexAttributeDescriptionCount = (uint32_t)viads.size();
  pvisci.pVertexAttributeDescriptions = viads.data();

  VkPipelineInputAssemblyStateCreateInfo piasci {};
  piasci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  switch (cfg.topo) {
  case L_TOPOLOGY_POINT:
    piasci.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    break;
  case L_TOPOLOGY_LINE:
    piasci.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    break;
  case L_TOPOLOGY_TRIANGLE:
    piasci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    break;
  default:
    panic("unexpected topology (", cfg.topo, ")");
  }
  piasci.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport {};
  viewport.x = 0;
  viewport.y = 0;
  viewport.width = (float)pass.viewport.extent.width;
  viewport.height = (float)pass.viewport.extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  VkRect2D scissor {};
  scissor.offset = pass.viewport.offset;
  scissor.extent = pass.viewport.extent;
  VkPipelineViewportStateCreateInfo pvsci {};
  pvsci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  pvsci.viewportCount = 1;
  pvsci.pViewports = &viewport;
  pvsci.scissorCount = 1;
  pvsci.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo prsci {};
  prsci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  prsci.cullMode = VK_CULL_MODE_NONE;
  prsci.frontFace = VK_FRONT_FACE_CLOCKWISE;
  prsci.polygonMode = VK_POLYGON_MODE_FILL;
  prsci.lineWidth = 1.0f;

  VkPipelineMultisampleStateCreateInfo pmsci {};
  pmsci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  pmsci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo pdssci {};
  pdssci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  pdssci.depthTestEnable = VK_TRUE;
  pdssci.depthWriteEnable = VK_TRUE;
  pdssci.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  pdssci.minDepthBounds = 0.0f;
  pdssci.maxDepthBounds = 1.0f;

  // TODO: (penguinliong) Support multiple attachments.
  std::array<VkPipelineColorBlendAttachmentState, 1> pcbass {};
  {
    VkPipelineColorBlendAttachmentState& pcbas = pcbass[0];
    pcbas.blendEnable = VK_FALSE;
    pcbas.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT |
      VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT |
      VK_COLOR_COMPONENT_A_BIT;
  }
  VkPipelineColorBlendStateCreateInfo pcbsci {};
  pcbsci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  pcbsci.attachmentCount = (uint32_t)pcbass.size();
  pcbsci.pAttachments = pcbass.data();

  VkPipelineDynamicStateCreateInfo pdsci {};
  pdsci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  pdsci.dynamicStateCount = 0;
  pdsci.pDynamicStates = nullptr;

  VkGraphicsPipelineCreateInfo gpci {};
  gpci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  gpci.stageCount = 2;
  gpci.pStages = psscis;
  gpci.pVertexInputState = &pvisci;
  gpci.pInputAssemblyState = &piasci;
  gpci.pTessellationState = nullptr;
  gpci.pViewportState = &pvsci;
  gpci.pRasterizationState = &prsci;
  gpci.pMultisampleState = &pmsci;
  gpci.pDepthStencilState = &pdssci;
  gpci.pColorBlendState = &pcbsci;
  gpci.pDynamicState = &pdsci;
  gpci.layout = pipe_layout;
  gpci.renderPass = pass.pass;
  gpci.subpass = 0;

  VkPipeline pipe;
  VK_ASSERT << vkCreateGraphicsPipelines(ctxt.dev, VK_NULL_HANDLE, 1, &gpci,
    nullptr, &pipe);

  log::debug("created graphics task '", cfg.label, "'");
  return Task {
    &ctxt, &pass, desc_set_layout, pipe_layout, pipe, cfg.rsc_tys,
    { vert_shader_mod, frag_shader_mod }, std::move(desc_pool_sizes), cfg.label
  };
}
void destroy_task(Task& task) {
  if (task.pipe != VK_NULL_HANDLE) {
    vkDestroyPipeline(task.ctxt->dev, task.pipe, nullptr);
    for (const VkShaderModule& shader_mod : task.shader_mods) {
      vkDestroyShaderModule(task.ctxt->dev, shader_mod, nullptr);
    }
    task.shader_mods.clear();
    vkDestroyPipelineLayout(task.ctxt->dev, task.pipe_layout, nullptr);
    vkDestroyDescriptorSetLayout(task.ctxt->dev, task.desc_set_layout, nullptr);

    log::debug("destroyed task '", task.label, "'");
    task = {};
  }
}



VkDescriptorPool _create_desc_pool(
  const Context& ctxt,
  const std::vector<VkDescriptorPoolSize>& desc_pool_sizes
) {
  VkDescriptorPoolCreateInfo dpci {};
  dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  dpci.poolSizeCount = static_cast<uint32_t>(desc_pool_sizes.size());
  dpci.pPoolSizes = desc_pool_sizes.data();
  dpci.maxSets = 1;

  VkDescriptorPool desc_pool;
  VK_ASSERT << vkCreateDescriptorPool(ctxt.dev, &dpci, nullptr, &desc_pool);
  return desc_pool;
}
VkDescriptorSet _alloc_desc_set(
  const Context& ctxt,
  VkDescriptorPool desc_pool,
  VkDescriptorSetLayout desc_set_layout
) {
  VkDescriptorSetAllocateInfo dsai {};
  dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dsai.descriptorPool = desc_pool;
  dsai.descriptorSetCount = 1;
  dsai.pSetLayouts = &desc_set_layout;

  VkDescriptorSet desc_set;
  VK_ASSERT << vkAllocateDescriptorSets(ctxt.dev, &dsai, &desc_set);
  return desc_set;
}
void _update_desc_set(
  const Context& ctxt,
  VkDescriptorSet desc_set,
  const std::vector<ResourceType>& rsc_tys,
  const std::vector<ResourceView>& rsc_views
) {
  std::vector<VkDescriptorBufferInfo> dbis;
  std::vector<VkDescriptorImageInfo> diis;
  std::vector<VkWriteDescriptorSet> wdss;

  auto push_dbi = [&](const BufferView& buf_view) {
    VkDescriptorBufferInfo dbi {};
    dbi.buffer = buf_view.buf->buf;
    dbi.offset = buf_view.offset;
    dbi.range = buf_view.size;
    dbis.emplace_back(std::move(dbi));

    log::debug("bound pool resource #", wdss.size(), " to buffer '",
      buf_view.buf->buf_cfg.label, "'");

    return dbis.back();
  };
  auto push_dii = [&](const ImageView& img_view, VkImageLayout layout) {
    VkDescriptorImageInfo dii {};
    dii.imageView = img_view.img->img_view;
    dii.imageLayout = layout;
    diis.emplace_back(std::move(dii));

    log::debug("bound pool resource #", wdss.size(), " to image '",
      img_view.img->img_cfg.label, "'");

    return diis.back();
  };

  wdss.reserve(rsc_views.size());
  for (uint32_t i = 0; i < rsc_views.size(); ++i) {
    const ResourceView& rsc_view = rsc_views[i];

    VkWriteDescriptorSet wds {};
    wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    wds.dstSet = desc_set;
    wds.dstBinding = i;
    wds.dstArrayElement = 0;
    wds.descriptorCount = 1;
    switch (rsc_tys[i]) {
    case L_RESOURCE_TYPE_UNIFORM_BUFFER:
      wds.pBufferInfo = &push_dbi(rsc_view.buf_view);
      wds.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      break;
    case L_RESOURCE_TYPE_STORAGE_BUFFER:
      wds.pBufferInfo = &push_dbi(rsc_view.buf_view);
      wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      break;
    case L_RESOURCE_TYPE_SAMPLED_IMAGE:
      wds.pImageInfo = &push_dii(rsc_view.img_view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
      wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      break;
    case L_RESOURCE_TYPE_STORAGE_IMAGE:
      wds.pImageInfo = &push_dii(rsc_view.img_view,
        VK_IMAGE_LAYOUT_GENERAL);
      wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      break;
    default: panic("unexpected resource type");
    }
    wdss.emplace_back(std::move(wds));
  }

  vkUpdateDescriptorSets(ctxt.dev, (uint32_t)wdss.size(), wdss.data(), 0,
    nullptr);
}
Invocation _create_invoke_common(
  const Task& task,
  const std::vector<ResourceView>& rsc_views
) {
  assert(task.rsc_tys.size() == rsc_views.size());

  const Context& ctxt = *task.ctxt;

  Invocation out {};
  if (task.desc_pool_sizes.size() > 0) {
    out.desc_pool = _create_desc_pool(ctxt, task.desc_pool_sizes);
    out.desc_set = _alloc_desc_set(ctxt, out.desc_pool, task.desc_set_layout);
    _update_desc_set(ctxt, out.desc_set, task.rsc_tys, rsc_views);
  }
  out.task = &task;

  return out;
}
Invocation create_comp_invoke(
  const Task& task,
  const ComputeInvocationConfig& cfg
) {
  const Context& ctxt = *task.ctxt;

  Invocation out = _create_invoke_common(task, cfg.rsc_views);
  out.submit_ty = L_SUBMIT_TYPE_COMPUTE;
  out.bind_pt = VK_PIPELINE_BIND_POINT_COMPUTE;

  InvocationComputeDetail comp_detail {};
  comp_detail.workgrp_count = cfg.workgrp_count;

  out.comp_detail =
    std::make_unique<InvocationComputeDetail>(std::move(comp_detail));

  log::debug("created compute invocation");
  return out;
}
Invocation create_graph_invoke(
  const Task& task,
  const GraphicsInvocationConfig& cfg
) {
  const Context& ctxt = *task.ctxt;

  Invocation out = _create_invoke_common(task, cfg.rsc_views);
  out.submit_ty = L_SUBMIT_TYPE_GRAPHICS;
  out.bind_pt = VK_PIPELINE_BIND_POINT_GRAPHICS;

  std::vector<VkBuffer> vert_bufs;
  std::vector<VkDeviceSize> vert_buf_offsets;
  vert_bufs.reserve(cfg.vert_bufs.size());
  vert_buf_offsets.reserve(cfg.vert_bufs.size());
  for (size_t i = 0; i < cfg.vert_bufs.size(); ++i) {
    const BufferView& vert_buf = cfg.vert_bufs[i];
    vert_bufs.emplace_back(vert_buf.buf->buf);
    vert_buf_offsets.emplace_back(vert_buf.offset);
  }

  InvocationGraphicsDetail graph_detail {};
  graph_detail.vert_bufs = std::move(vert_bufs);
  graph_detail.vert_buf_offsets = std::move(vert_buf_offsets);
  graph_detail.idx_buf = cfg.idx_buf.buf->buf;
  graph_detail.idx_buf_offset = cfg.idx_buf.offset;
  graph_detail.ninst = cfg.ninst;
  graph_detail.nvert = cfg.nvert;
  graph_detail.nidx = cfg.nidx;

  out.graph_detail =
    std::make_unique<InvocationGraphicsDetail>(std::move(graph_detail));

  log::debug("created graphics invocation");
  return out;
}
void destroy_invoke(Invocation& invoke) {
  if (invoke.desc_pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(invoke.task->ctxt->dev, invoke.desc_pool, nullptr);
    log::debug("destroyed invocation");
  }
  invoke = {};
}



VkSemaphore _create_sema(const Context& ctxt) {
  VkSemaphoreCreateInfo sci {};
  sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkSemaphore sema;
  VK_ASSERT << vkCreateSemaphore(ctxt.dev, &sci, nullptr, &sema);
  return sema;
}
VkFence _create_fence(const Context& ctxt) {
  VkFenceCreateInfo fci {};
  fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  VkFence fence;
  VK_ASSERT << vkCreateFence(ctxt.dev, &fci, nullptr, &fence);
  return fence;
}

VkCommandPool _create_cmd_pool(const Context& ctxt, SubmitType submit_ty) {
  VkCommandPoolCreateInfo cpci {};
  cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cpci.queueFamilyIndex = ctxt.get_submit_ty_qfam_idx(submit_ty);

  VkCommandPool cmd_pool;
  VK_ASSERT << vkCreateCommandPool(ctxt.dev, &cpci, nullptr, &cmd_pool);

  return cmd_pool;
}
VkCommandBuffer _alloc_cmdbuf(
  const Context& ctxt,
  VkCommandPool cmd_pool,
  VkCommandBufferLevel level
) {
  VkCommandBufferAllocateInfo cbai {};
  cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cbai.level = level;
  cbai.commandBufferCount = 1;
  cbai.commandPool = cmd_pool;

  VkCommandBuffer cmdbuf;
  VK_ASSERT << vkAllocateCommandBuffers(ctxt.dev, &cbai, &cmdbuf);

  return cmdbuf;
}



struct TransactionLike {
  const Context* ctxt;
  std::vector<TransactionSubmitDetail> submit_details;
  VkCommandBufferLevel level;
};
void _begin_cmdbuf(const TransactionSubmitDetail& submit_detail) {
  VkCommandBufferInheritanceInfo cbii {};
  cbii.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

  VkCommandBufferBeginInfo cbbi {};
  cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  if (submit_detail.submit_ty == L_SUBMIT_TYPE_GRAPHICS) {
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
  }
  cbbi.pInheritanceInfo = &cbii;
  VK_ASSERT << vkBeginCommandBuffer(submit_detail.cmdbuf, &cbbi);
}
void _end_cmdbuf(const TransactionSubmitDetail& submit_detail) {
  VK_ASSERT << vkEndCommandBuffer(submit_detail.cmdbuf);
}

void _push_transact_submit_detail(
  const Context& ctxt,
  std::vector<TransactionSubmitDetail>& submit_details,
  SubmitType submit_ty,
  VkCommandBufferLevel level
) {
  auto cmd_pool = _create_cmd_pool(ctxt, submit_ty);
  auto cmdbuf = _alloc_cmdbuf(ctxt, cmd_pool, level);

  TransactionSubmitDetail submit_detail {};
  submit_detail.submit_ty = submit_ty;
  submit_detail.cmd_pool = cmd_pool;
  submit_detail.cmdbuf = cmdbuf;
  submit_detail.wait_sema = submit_details.empty() ?
    VK_NULL_HANDLE : submit_details.back().signal_sema;
  submit_detail.signal_sema = _create_sema(ctxt);

  submit_details.emplace_back(std::move(submit_detail));
}
void _clear_transact_submit_detail(
  const Context& ctxt,
  std::vector<TransactionSubmitDetail>& submit_details
) {
  for (auto& submit_detail : submit_details) {
    vkDestroySemaphore(ctxt.dev, submit_detail.signal_sema, nullptr);
    vkDestroyCommandPool(ctxt.dev, submit_detail.cmd_pool, nullptr);
  }
  submit_details.clear();
}
void _submit_transact_submit_detail(
  const Context& ctxt,
  const TransactionSubmitDetail& submit_detail,
  VkFence fence
) {
  VkPipelineStageFlags stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

  VkSubmitInfo submit_info {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &submit_detail.cmdbuf;
  submit_info.signalSemaphoreCount = 1;
  if (submit_detail.wait_sema != VK_NULL_HANDLE) {
    // Wait for the last submitted command buffer on the device side.
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &submit_detail.wait_sema;
    submit_info.pWaitDstStageMask = &stage_mask;
  }
  submit_info.pSignalSemaphores = &submit_detail.signal_sema;

  // Finish recording and submit the command buffer to the device.
  VkQueue queue = ctxt.get_submit_ty_queue(submit_detail.submit_ty);
  VK_ASSERT << vkQueueSubmit(queue, 1, &submit_info, fence);
}
VkCommandBuffer _get_cmdbuf(
  TransactionLike& transact,
  SubmitType submit_ty
) {
  if (submit_ty == L_SUBMIT_TYPE_ANY) {
    assert(!transact.submit_details.empty(), "cannot infer submit type for "
      "submit-type-independent command");
    submit_ty = transact.submit_details.back().submit_ty;
  }
  const auto& submit_detail = transact.ctxt->get_submit_detail(submit_ty);
  auto queue = submit_detail.queue;
  auto qfam_idx = submit_detail.qfam_idx;

  if (!transact.submit_details.empty()) {
    // Do nothing if the submit type is unchanged. It means that the commands
    // can still be fed into the last command buffer.
    auto& last_submit = transact.submit_details.back();
    if (submit_ty == last_submit.submit_ty) {
      return last_submit.cmdbuf;
    }

    // Otherwise, end the command buffer and, it it's a primary command buffer,
    // submit the recorded commands.
    _end_cmdbuf(last_submit);
    if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
      _submit_transact_submit_detail(*transact.ctxt,
        transact.submit_details.back(), VK_NULL_HANDLE);
    }
  }

  _push_transact_submit_detail(*transact.ctxt, transact.submit_details,
    submit_ty, transact.level);
  _begin_cmdbuf(transact.submit_details.back());
  return transact.submit_details.back().cmdbuf;
}

void _record_cmd_set_submit_ty(
  TransactionLike& transact,
  const Command& cmd
) {
  SubmitType submit_ty = cmd.cmd_set_submit_ty.submit_ty;
  _get_cmdbuf(transact, submit_ty);
  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    log::debug("command drain submit type is set");
  }
}
void _record_cmd_inline_transact(
  TransactionLike& transact,
  const Command& cmd
) {
  assert(transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    "nested inline transaction is not allowed");
  const auto& in = cmd.cmd_inline_transact;
  const auto& subtransact = *in.transact;

  for (auto i = 0; i < subtransact.submit_details.size(); ++i) {
    const auto& submit_detail = subtransact.submit_details[i];
    auto cmdbuf = _get_cmdbuf(transact, submit_detail.submit_ty);

    vkCmdExecuteCommands(cmdbuf, 1, &submit_detail.cmdbuf);
  }

  log::debug("scheduled inline transaction '",
    cmd.cmd_inline_transact.transact->label, "'");
}

void _record_cmd_copy_buf2img(TransactionLike& transact, const Command& cmd) {
  const auto& in = cmd.cmd_copy_buf2img;
  const auto& src = in.src;
  const auto& dst = in.dst;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  VkBufferImageCopy bic {};
  bic.bufferOffset = src.offset;
  bic.bufferRowLength = 0;
  bic.bufferImageHeight = (uint32_t)dst.img->img_cfg.height;
  bic.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bic.imageSubresource.mipLevel = 0;
  bic.imageSubresource.baseArrayLayer = 0;
  bic.imageSubresource.layerCount = 1;
  bic.imageOffset.x = dst.x_offset;
  bic.imageOffset.y = dst.y_offset;
  bic.imageExtent.width = dst.width;
  bic.imageExtent.height = dst.height;
  bic.imageExtent.depth = 1;

  vkCmdCopyBufferToImage(cmdbuf, src.buf->buf, dst.img->img,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bic);
  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    log::debug("scheduled copy from buffer '", src.buf->buf_cfg.label,
      "' to image '", dst.img->img_cfg.label, "'");
  }
}
void _record_cmd_copy_img2buf(TransactionLike& transact, const Command& cmd) {
  const auto& in = cmd.cmd_copy_img2buf;
  const auto& src = in.src;
  const auto& dst = in.dst;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  VkBufferImageCopy bic {};
  bic.bufferOffset = dst.offset;
  bic.bufferRowLength = 0;
  bic.bufferImageHeight = static_cast<uint32_t>(src.img->img_cfg.height);
  bic.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bic.imageSubresource.mipLevel = 0;
  bic.imageSubresource.baseArrayLayer = 0;
  bic.imageSubresource.layerCount = 1;
  bic.imageOffset.x = src.x_offset;
  bic.imageOffset.y = src.y_offset;
  bic.imageExtent.width = src.width;
  bic.imageExtent.height = src.height;
  bic.imageExtent.depth = 1;

  vkCmdCopyImageToBuffer(cmdbuf, src.img->img,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst.buf->buf, 1, &bic);
  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    log::debug("scheduled copy from image '", src.img->img_cfg.label,
      "' to buffer '", dst.buf->buf_cfg.label, "'");
  }
}
void _record_cmd_copy_buf(TransactionLike& transact, const Command& cmd) {
  const auto& in = cmd.cmd_copy_buf;
  const auto& src = in.src;
  const auto& dst = in.dst;
  assert(src.size == dst.size, "buffer copy size mismatched");
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  VkBufferCopy bc {};
  bc.srcOffset = src.offset;
  bc.dstOffset = dst.offset;
  bc.size = dst.size;

  vkCmdCopyBuffer(cmdbuf, src.buf->buf, dst.buf->buf, 1, &bc);
  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    log::debug("scheduled copy from buffer '", src.buf->buf_cfg.label,
      "' to buffer '", dst.buf->buf_cfg.label, "'");
  }
}
void _record_cmd_copy_img(TransactionLike& transact, const Command& cmd) {
  const auto& in = cmd.cmd_copy_img;
  const auto& src = in.src;
  const auto& dst = in.dst;
  assert(src.width == dst.width && src.height == dst.height,
    "image copy size mismatched");
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  VkImageCopy ic {};
  ic.srcOffset.x = src.x_offset;
  ic.srcOffset.y = src.y_offset;
  ic.dstOffset.x = dst.x_offset;
  ic.dstOffset.y = dst.y_offset;
  ic.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  ic.srcSubresource.baseArrayLayer = 0;
  ic.srcSubresource.layerCount = 1;
  ic.srcSubresource.mipLevel = 0;
  ic.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  ic.dstSubresource.baseArrayLayer = 0;
  ic.dstSubresource.layerCount = 1;
  ic.dstSubresource.mipLevel = 0;
  ic.extent.width = dst.width;
  ic.extent.height = dst.height;
  ic.extent.depth = 1;

  vkCmdCopyImage(cmdbuf, src.img->img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    dst.img->img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ic);
  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    log::debug("scheduled copy from image '", src.img->img_cfg.label,
      "' to image '", dst.img->img_cfg.label, "'");
  }
}

void _record_cmd_invoke(TransactionLike& transact, const Command& cmd) {
  const auto& in = cmd.cmd_invoke;
  const auto& invoke = *in.invoke;
  const auto& task = *invoke.task;

  VkCommandBuffer cmdbuf = _get_cmdbuf(transact, invoke.submit_ty);
  
  vkCmdBindPipeline(cmdbuf, invoke.bind_pt, task.pipe);
  if (invoke.desc_set != VK_NULL_HANDLE) {
    vkCmdBindDescriptorSets(cmdbuf, invoke.bind_pt, task.pipe_layout, 0, 1,
      &invoke.desc_set, 0, nullptr);
  }

  switch (invoke.bind_pt) {
  case VK_PIPELINE_BIND_POINT_COMPUTE:
    assert(invoke.comp_detail != nullptr);
    {
      const InvocationComputeDetail& comp_detail = *invoke.comp_detail;
      const DispatchSize& workgrp_count = comp_detail.workgrp_count;
      vkCmdDispatch(cmdbuf, workgrp_count.x, workgrp_count.y, workgrp_count.z);
    }
    break;
  case VK_PIPELINE_BIND_POINT_GRAPHICS:
    assert(invoke.graph_detail != nullptr);
    {
      const InvocationGraphicsDetail& graph_detail = *invoke.graph_detail;
      vkCmdBindVertexBuffers(cmdbuf, 0, (uint32_t)graph_detail.vert_bufs.size(),
        graph_detail.vert_bufs.data(), graph_detail.vert_buf_offsets.data());
      if (invoke.graph_detail->nidx != 0) {
        vkCmdBindIndexBuffer(cmdbuf, graph_detail.idx_buf,
          graph_detail.idx_buf_offset, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(cmdbuf, graph_detail.nidx, graph_detail.ninst,
          0, 0, 0);
      } else {
        vkCmdDraw(cmdbuf, graph_detail.nvert, graph_detail.ninst, 0, 0);
      }
    }
    break;
  default:
    unreachable("unexpected submit type");
  }

  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    log::debug("scheduled invocation '", task.label, "' for execution");
  }
}

void _record_cmd_write_timestamp(
  TransactionLike& transact,
  const Command& cmd
) {
  const auto& in = cmd.cmd_write_timestamp;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  auto query_pool = in.timestamp->query_pool;
  vkCmdResetQueryPool(cmdbuf, query_pool, 0, 1);
  vkCmdWriteTimestamp(cmdbuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, query_pool, 0);

  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    log::debug("scheduled timestamp write");
  }
}

void _make_buf_barrier_params(
  BufferUsage usage,
  VkAccessFlags& access,
  VkPipelineStageFlags& stage
) {
  switch (usage) {
  case L_BUFFER_USAGE_NONE:
    return; // Keep their original values.
  case L_BUFFER_USAGE_STAGING_BIT:
    access = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    break;
  case L_BUFFER_USAGE_UNIFORM_BIT:
    access = VK_ACCESS_UNIFORM_READ_BIT;
    stage =
      VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    break;
  case L_BUFFER_USAGE_STORAGE_BIT:
    access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    stage =
      VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    break;
  case L_BUFFER_USAGE_VERTEX_BIT:
    access = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
    break;
  case L_BUFFER_USAGE_INDEX_BIT:
    access = VK_ACCESS_INDEX_READ_BIT;
    stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
    break;
  default:
    panic("destination usage cannot be a set of bits");
  }
}

void _make_img_barrier_params(
  ImageUsage usage,
  VkAccessFlags& access,
  VkPipelineStageFlags& stage,
  VkImageLayout& layout
) {
  switch (usage) {
  case L_IMAGE_USAGE_NONE:
    return; // Keep their original values.
  case L_IMAGE_USAGE_STAGING_BIT:
    access = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    break;
  case L_IMAGE_USAGE_SAMPLED_BIT:
    access = VK_ACCESS_SHADER_READ_BIT;
    stage =
      VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    break;
  case L_IMAGE_USAGE_STORAGE_BIT:
    access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    stage =
      VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    layout = VK_IMAGE_LAYOUT_GENERAL;
    break;
  case L_IMAGE_USAGE_ATTACHMENT_BIT:
    access =
      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | // Blending does this.
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    break;
  case L_IMAGE_USAGE_SUBPASS_DATA_BIT:
    access = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    break;
  case L_IMAGE_USAGE_PRESENT_BIT:
    access = 0;
    stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    break;
  default:
    panic("destination usage cannot be a set of bits");
  }
}

void _record_cmd_buf_barrier(
  TransactionLike& transact,
  const Command& cmd
) {
  const auto& in = cmd.cmd_buf_barrier;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  BufferUsage src_usage = in.src_usage;
  BufferUsage dst_usage = in.dst_usage;

  VkAccessFlags src_access = 0;
  VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  _make_buf_barrier_params(src_usage, src_access, src_stage);

  VkAccessFlags dst_access = 0;
  VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  _make_buf_barrier_params(dst_usage, dst_access, dst_stage);

  VkBufferMemoryBarrier bmb {};
  bmb.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bmb.buffer = in.buf->buf;
  bmb.srcAccessMask = src_access;
  bmb.dstAccessMask = dst_access;
  bmb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bmb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bmb.offset = 0;
  bmb.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(
    cmdbuf,
    src_stage,
    dst_stage,
    0,
    0, nullptr,
    1, &bmb,
    0, nullptr);
  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    log::debug("scheduled buffer barrier");
  }
}
void _record_cmd_img_barrier(
  TransactionLike& transact,
  const Command& cmd
) {
  const auto& in = cmd.cmd_img_barrier;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  ImageUsage src_usage = in.src_usage;
  ImageUsage dst_usage = in.dst_usage;

  VkAccessFlags src_access = 0;
  VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  VkImageLayout src_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  _make_img_barrier_params(src_usage, src_access, src_stage, src_layout);

  VkAccessFlags dst_access = 0;
  VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkImageLayout dst_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  _make_img_barrier_params(dst_usage, dst_access, dst_stage, dst_layout);

  VkImageMemoryBarrier imb {};
  imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imb.image = in.img->img;
  imb.srcAccessMask = src_access;
  imb.dstAccessMask = dst_access;
  imb.oldLayout = src_layout;
  imb.newLayout = dst_layout;
  imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imb.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imb.subresourceRange.baseArrayLayer = 0;
  imb.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
  imb.subresourceRange.baseMipLevel = 0;
  imb.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

  vkCmdPipelineBarrier(
    cmdbuf,
    src_stage,
    dst_stage,
    0,
    0, nullptr,
    0, nullptr,
    1, &imb);
  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    log::debug("scheduled image barrier");
  }
}

void _record_cmd_begin_pass(TransactionLike& transact, const Command& cmd) {
  assert(transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  const auto& in = cmd.cmd_begin_pass;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_GRAPHICS);
  const auto& pass = *in.pass;

  VkRenderPassBeginInfo rpbi {};
  rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rpbi.renderPass = pass.pass;
  rpbi.framebuffer = pass.framebuf;
  rpbi.renderArea.extent = pass.viewport.extent;
  rpbi.clearValueCount = (uint32_t)pass.clear_values.size();
  rpbi.pClearValues = pass.clear_values.data();
  VkSubpassContents sc = in.draw_inline ?
    VK_SUBPASS_CONTENTS_INLINE :
    VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
  vkCmdBeginRenderPass(cmdbuf, &rpbi, sc);

  log::debug("scheduled render pass begin");
}
void _record_cmd_end_pass(TransactionLike& transact, const Command& cmd) {
  assert(transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  const auto& in = cmd.cmd_end_pass;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_GRAPHICS);

  vkCmdEndRenderPass(cmdbuf);

  log::debug("scheduled render pass end");
}

// TODO: (penguinliong) Check these pipeline stages.
void _make_depth_img_barrier_params(
  DepthImageUsage usage,
  VkAccessFlags& access,
  VkPipelineStageFlags& stage,
  VkImageLayout& layout
) {
  switch (usage) {
  case L_DEPTH_IMAGE_USAGE_NONE:
    return; // Keep their original values.
  case L_DEPTH_IMAGE_USAGE_SAMPLED_BIT:
    access = VK_ACCESS_SHADER_READ_BIT;
    stage =
      VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    break;
  case L_DEPTH_IMAGE_USAGE_ATTACHMENT_BIT:
    access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    stage = 
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
      VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    break;
  case L_DEPTH_IMAGE_USAGE_SUBPASS_DATA_BIT:
    access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    stage =
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
      VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    break;
  default:
    panic("destination usage cannot be a set of bits");
  }
}
void _record_cmd_depth_img_barrier(
  TransactionLike& transact,
  const Command& cmd
) {
  const auto& in = cmd.cmd_depth_img_barrier;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  VkAccessFlags src_access = 0;
  VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  VkImageLayout src_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  _make_depth_img_barrier_params(in.src_usage, src_access, src_stage,
    src_layout);

  VkAccessFlags dst_access = 0;
  VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkImageLayout dst_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  _make_depth_img_barrier_params(in.dst_usage, dst_access, dst_stage,
    dst_layout);

  VkImageMemoryBarrier imb {};
  imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imb.image = in.depth_img->img;
  imb.srcAccessMask = src_access;
  imb.dstAccessMask = dst_access;
  imb.oldLayout = src_layout;
  imb.newLayout = dst_layout;
  imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imb.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  imb.subresourceRange.baseArrayLayer = 0;
  imb.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
  imb.subresourceRange.baseMipLevel = 0;
  imb.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

  vkCmdPipelineBarrier(
    cmdbuf,
    src_stage,
    dst_stage,
    0,
    0, nullptr,
    0, nullptr,
    1, &imb);

  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    log::debug("schedule depth image barrier");
  }
}

// Returns whether the submit queue to submit has changed.
void _record_cmd(TransactionLike& transact, const Command& cmd) {
  switch (cmd.cmd_ty) {
  case L_COMMAND_TYPE_SET_SUBMIT_TYPE:
    _record_cmd_set_submit_ty(transact, cmd);
    break;
  case L_COMMAND_TYPE_INLINE_TRANSACTION:
    _record_cmd_inline_transact(transact, cmd);
    break;
  case L_COMMAND_TYPE_COPY_BUFFER_TO_IMAGE:
    _record_cmd_copy_buf2img(transact, cmd);
    break;
  case L_COMMAND_TYPE_COPY_IMAGE_TO_BUFFER:
    _record_cmd_copy_img2buf(transact, cmd);
    break;
  case L_COMMAND_TYPE_COPY_BUFFER:
    _record_cmd_copy_buf(transact, cmd);
    break;
  case L_COMMAND_TYPE_COPY_IMAGE:
    _record_cmd_copy_img(transact, cmd);
    break;
  case L_COMMAND_TYPE_INVOKE:
    _record_cmd_invoke(transact, cmd);
    break;
  case L_COMMAND_TYPE_WRITE_TIMESTAMP:
    _record_cmd_write_timestamp(transact, cmd);
    break;
  case L_COMMAND_TYPE_BUFFER_BARRIER:
    _record_cmd_buf_barrier(transact, cmd);
    break;
  case L_COMMAND_TYPE_IMAGE_BARRIER:
    _record_cmd_img_barrier(transact, cmd);
    break;
  case L_COMMAND_TYPE_BEGIN_RENDER_PASS:
    _record_cmd_begin_pass(transact, cmd);
    break;
  case L_COMMAND_TYPE_END_RENDER_PASS:
    _record_cmd_end_pass(transact, cmd);
    break;
  case L_COMMAND_TYPE_DEPTH_IMAGE_BARRIER:
    _record_cmd_depth_img_barrier(transact, cmd);
    break;
  default:
    log::warn("ignored unknown command: ", cmd.cmd_ty);
    break;
  }
}



CommandDrain create_cmd_drain(const Context& ctxt) {
  auto fence = _create_fence(ctxt);
  log::debug("created command drain");
  return CommandDrain { &ctxt, {}, fence };
}
void destroy_cmd_drain(CommandDrain& cmd_drain) {
  if (cmd_drain.fence != VK_NULL_HANDLE) {
    _clear_transact_submit_detail(*cmd_drain.ctxt, cmd_drain.submit_details);
    vkDestroyFence(cmd_drain.ctxt->dev, cmd_drain.fence, nullptr);
    cmd_drain = {};
    log::debug("destroyed command drain");
  }
}
void submit_cmds(
  CommandDrain& cmd_drain,
  const Command* cmds,
  size_t ncmd
) {
  assert(ncmd > 0, "cannot submit empty command buffer");

  TransactionLike transact {};
  transact.ctxt = cmd_drain.ctxt;
  transact.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  util::Timer timer {};
  timer.tic();
  for (auto i = 0; i < ncmd; ++i) {
    log::debug("recording ", i, "th command");
    _record_cmd(transact, cmds[i]);
  }
  cmd_drain.submit_details = std::move(transact.submit_details);
  timer.toc();

  _end_cmdbuf(cmd_drain.submit_details.back());
  _submit_transact_submit_detail(*cmd_drain.ctxt,
    cmd_drain.submit_details.back(), cmd_drain.fence);

  log::debug("submitted transaction for execution, command recording took ",
    timer.us(), "us");
}
void _reset_cmd_drain(CommandDrain& cmd_drain) {
  _clear_transact_submit_detail(*cmd_drain.ctxt, cmd_drain.submit_details);
  VK_ASSERT << vkResetFences(cmd_drain.ctxt->dev, 1, &cmd_drain.fence);
}
void wait_cmd_drain(CommandDrain& cmd_drain) {
  const uint32_t SPIN_INTERVAL = 3000;

  util::Timer wait_timer {};
  wait_timer.tic();
  for (VkResult err;;) {
    err = vkWaitForFences(cmd_drain.ctxt->dev, 1, &cmd_drain.fence, VK_TRUE,
        SPIN_INTERVAL);
    if (err == VK_TIMEOUT) {
      // log::warn("timeout after 3000ns");
    } else {
      VK_ASSERT << err;
      break;
    }
  }
  wait_timer.toc();

  util::Timer reset_timer {};
  reset_timer.tic();

  _reset_cmd_drain(cmd_drain);

  reset_timer.toc();

  log::debug("command drain returned after ", wait_timer.us(), "us since the "
    "wait started (spin interval = ", SPIN_INTERVAL / 1000.0, "us; resource "
    "recycling took ", reset_timer.us(), "us)");
}



Transaction create_transact(
  const std::string& label,
  const Context& ctxt,
  const Command* cmds,
  size_t ncmd
) {
  TransactionLike transact {};
  transact.ctxt = &ctxt;
  transact.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  for (auto i = 0; i < ncmd; ++i) {
    _record_cmd(transact, cmds[i]);
  }
  _end_cmdbuf(transact.submit_details.back());

  log::debug("created transaction");
  return Transaction { label, &ctxt, std::move(transact.submit_details) };
}
void destroy_transact(Transaction& transact) {
  _clear_transact_submit_detail(*transact.ctxt, transact.submit_details);
  transact = {};
  log::debug("destroyed transaction");
}



Timestamp create_timestamp(const Context& ctxt) {
  VkQueryPoolCreateInfo qpci {};
  qpci.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  qpci.queryCount = 1;
  qpci.queryType = VK_QUERY_TYPE_TIMESTAMP;

  VkQueryPool query_pool;
  VK_ASSERT << vkCreateQueryPool(ctxt.dev, &qpci, nullptr, &query_pool);

  log::debug("created timestamp");
  return Timestamp { &ctxt, query_pool };
}
void destroy_timestamp(Timestamp& timestamp) {
  if (timestamp.query_pool != VK_NULL_HANDLE) {
    vkDestroyQueryPool(timestamp.ctxt->dev, timestamp.query_pool, nullptr);
    timestamp = {};
    log::debug("destroyed timestamp");
  }
}
double get_timestamp_result_us(const Timestamp& timestamp) {
  uint64_t t;
  VK_ASSERT << vkGetQueryPoolResults(timestamp.ctxt->dev, timestamp.query_pool,
    0, 1, sizeof(uint64_t), &t, sizeof(uint64_t),
    VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT); // Wait till ready.
  double ns_per_tick = timestamp.ctxt->physdev_prop.limits.timestampPeriod;
  return t * ns_per_tick / 1000.0;
}



namespace ext {

std::vector<uint8_t> load_code(const std::string& prefix) {
  auto path = prefix + ".comp.spv";
  return util::load_file(path.c_str());
}

} // namespace ext

} // namespace HAL_IMPL_NAMESPACE

} // namespace liong

#endif // GFT_WITH_VULKAN
