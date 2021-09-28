#include <map>
#include <set>
#include <algorithm>
#include "vk.hpp"
#include "assert.hpp"
#include "util.hpp"
#include "log.hpp"
#define HAL_IMPL_NAMESPACE vk
#include "scoped-hal-impl.hpp"
#undef HAL_IMPL_NAMESPACE

namespace liong {

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



namespace vk {

VkInstance inst = nullptr;
std::vector<VkPhysicalDevice> physdevs {};
std::vector<std::string> physdev_descs {};



void initialize() {
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

  // Enable all extensions by default.
  std::vector<const char*> inst_ext_names;
  inst_ext_names.reserve(ninst_ext);
  for (const auto& inst_ext : inst_exts) {
    inst_ext_names.emplace_back(inst_ext.extensionName);
  }
  liong::log::info("enabled instance extensions: ", liong::util::join(", ", inst_ext_names));

  VkInstanceCreateInfo ici {};
  ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  ici.pApplicationInfo = &app_info;
  ici.enabledExtensionCount = inst_ext_names.size();
  ici.ppEnabledExtensionNames = inst_ext_names.data();

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
}
std::string desc_dev(uint32_t idx) {
  return idx < physdev_descs.size() ? physdev_descs[idx] : std::string {};
}


// Get memory type priority based on the host access pattern. Higher the better.
uint32_t _get_mem_prior(
  MemoryAccess host_access,
  VkMemoryPropertyFlags mem_prop
) {
  if (host_access == L_MEMORY_ACCESS_NONE) {
    if (mem_prop & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
      return 1;
    } else {
      return 0;
    }
  } else if (host_access == L_MEMORY_ACCESS_READ_ONLY) {
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
      if (mem_prop == PRIORITY_LUT[i]) { return PRIORITY_LUT.size() - i; }
    }
    return 0;
  } else if (host_access == L_MEMORY_ACCESS_WRITE_ONLY) {
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
      if (mem_prop == PRIORITY_LUT[i]) { return PRIORITY_LUT.size() - i; }
    }
    return 0;
  } else if (host_access == L_MEMORY_ACCESS_READ_WRITE) {
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
      if (mem_prop == PRIORITY_LUT[i]) { return PRIORITY_LUT.size() - i; }
    }
    return 0;
  } else {
    liong::panic("unexpected host access pattern");
    return 0;
  }
}
Context create_ctxt(const ContextConfig& cfg) {
  auto physdev = physdevs[cfg.dev_idx];

  VkPhysicalDeviceFeatures feat;
  vkGetPhysicalDeviceFeatures(physdev, &feat);

  VkPhysicalDeviceProperties physdev_prop;
  vkGetPhysicalDeviceProperties(physdev, &physdev_prop);

  if (physdev_prop.limits.timestampComputeAndGraphics == VK_FALSE) {
    liong::log::warn("context '", cfg.label, "' device does not support "
      "timestamps, the following command won't be available: WRITE_TIMESTAMP");
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
      liong::log::warn("ignored queue family #", i, " with zero queue count");
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
    liong::log::info("discovered queue families #", i, ": ",
      liong::util::join(" | ", qfam_cap_lit));

    uint32_t nset_bit = liong::util::count_set_bits(queue_flags);
    auto it = qfam_map.find(nset_bit);
    if (it == qfam_map.end()) {
      it = qfam_map.emplace_hint(it,
        std::make_pair(nset_bit, std::vector<QueueFamilyTrait>{}));
    }
    it->second.emplace_back(QueueFamilyTrait { i, queue_flags });
  }
  liong::assert(!qfam_props.empty(), "cannot find any queue family on device #",
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
      liong::log::warn("cannot find a suitable queue family for ",
        submit_ty_queue_req.submit_ty_name, ", the following commands won't be "
        "available: ", liong::util::join(", ", submit_ty_queue_req.cmd_names));
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
  liong::log::info("enabled device extensions: ", liong::util::join(", ", dev_ext_names));

  VkDeviceCreateInfo dci {};
  dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  dci.pEnabledFeatures = &feat;
  dci.queueCreateInfoCount = static_cast<uint32_t>(dqcis.size());
  dci.pQueueCreateInfos = dqcis.data();
  dci.enabledExtensionCount = dev_ext_names.size();
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
    liong::util::arrange(mem_prop.memoryTypeCount),
    liong::util::arrange(mem_prop.memoryTypeCount),
    liong::util::arrange(mem_prop.memoryTypeCount),
    liong::util::arrange(mem_prop.memoryTypeCount),
  };
  for (int host_access = 0; host_access < 4; ++host_access) {
    auto& mem_ty_idxs = mem_ty_idxs_by_host_access[host_access];
    std::vector<uint32_t> priors = liong::util::map<uint32_t, uint32_t>(
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
  sci.magFilter = VK_FILTER_NEAREST;
  sci.minFilter = VK_FILTER_NEAREST;
  sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sci.unnormalizedCoordinates = VK_TRUE;

  VkSampler fast_samp;
  VK_ASSERT << vkCreateSampler(dev, &sci, nullptr, &fast_samp);

  liong::log::info("created vulkan context '", cfg.label, "' on device #",
    cfg.dev_idx, ": ", physdev_descs[cfg.dev_idx]);
  return Context {
    dev, physdev, std::move(physdev_prop), std::move(submit_details),
    std::move(queue_allocs), std::move(mem_ty_idxs_by_host_access),
    fast_samp, cfg
  };
}
void destroy_ctxt(Context& ctxt) {
  vkDestroySampler(ctxt.dev, ctxt.fast_samp, nullptr);
  vkDestroyDevice(ctxt.dev, nullptr);
  liong::log::info("destroyed vulkan context '", ctxt.ctxt_cfg.label, "'");
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

  liong::log::info("created buffer '", buf_cfg.label, "'");
  return Buffer { &ctxt, devmem, buf, buf_cfg };
}
void destroy_buf(Buffer& buf) {
  vkDestroyBuffer(buf.ctxt->dev, buf.buf, nullptr);
  vkFreeMemory(buf.ctxt->dev, buf.devmem, nullptr);
  liong::log::info("destroyed buffer '", buf.buf_cfg.label, "'");
  buf = {};
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
  liong::log::info("mapped buffer '", buf.buf->buf_cfg.label, "' from ",
    buf.offset, " to ", buf.offset + buf.size);
}
void unmap_buf_mem(
  const BufferView& buf,
  void* mapped
) {
  vkUnmapMemory(buf.buf->ctxt->dev, buf.buf->devmem);
  liong::log::info("unmapped buffer '", buf.buf->buf_cfg.label, "'");
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
    liong::assert((img_cfg.usage & (~L_IMAGE_USAGE_STAGING_BIT)) == 0,
      "staging image can only be used for transfer");
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
  VK_ASSERT << vkGetPhysicalDeviceImageFormatProperties(
    physdevs[ctxt.ctxt_cfg.dev_idx], fmt, VK_IMAGE_TYPE_2D,
    VK_IMAGE_TILING_OPTIMAL, usage, 0, &ifp);

  VkImageLayout layout = is_staging_img ?
    VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;

  VkImageCreateInfo ici {};
  ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ici.imageType = VK_IMAGE_TYPE_2D;
  ici.format = fmt;
  ici.extent.width = (uint32_t)img_cfg.ncol;
  ici.extent.height = (uint32_t)img_cfg.nrow;
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

  liong::log::info("created image '", img_cfg.label, "'");
  uint32_t qfam_idx = ctxt.get_submit_ty_qfam_idx(init_submit_ty);
  return Image {
    &ctxt, devmem, img, img_view, img_cfg, is_staging_img
  };
}
void destroy_img(Image& img) {
  vkDestroyImageView(img.ctxt->dev, img.img_view, nullptr);
  vkDestroyImage(img.ctxt->dev, img.img, nullptr);
  vkFreeMemory(img.ctxt->dev, img.devmem, nullptr);

  liong::log::info("destroyed image '", img.img_cfg.label, "'");
  img = {};
}
const ImageConfig& get_img_cfg(const Image& img) {
  return img.img_cfg;
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

  liong::log::info("mapped image '", img.img->img_cfg.label, "' from (",
    img.col_offset, ", ", img.row_offset, ") to (", img.col_offset + img.ncol,
    ", ", img.row_offset + img.nrow, ")");
}
void unmap_img_mem(
  const ImageView& img,
  void* mapped
) {
  vkUnmapMemory(img.img->ctxt->dev, img.img->devmem);
  liong::log::info("unmapped image '", img.img->img_cfg.label, "'");
}




VkDescriptorSetLayout _create_desc_set_layout(
  const Context& ctxt,
  const ResourceType* rsc_tys,
  size_t nrsc_ty,
  std::vector<VkDescriptorPoolSize>& desc_pool_sizes
) {
  std::vector<VkDescriptorSetLayoutBinding> dslbs;
  std::map<VkDescriptorType, uint32_t> desc_counter;
  for (auto i = 0; i < nrsc_ty; ++i) {
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
      liong::panic("unexpected resource type");
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
    cfg.rsc_tys, cfg.nrsc_ty, desc_pool_sizes);
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

  liong::log::info("created compute task '", cfg.label, "'");
  return Task {
    &ctxt, desc_set_layout, pipe_layout, pipe, VK_NULL_HANDLE, { shader_mod },
    std::move(desc_pool_sizes), cfg.label,
  };
}
VkRenderPass _create_pass(
  const Context& ctxt
) {
  std::array<VkAttachmentReference, 1> ars {
    { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
  };
  std::array<VkAttachmentDescription, 1> ads {};
  {
    VkAttachmentDescription& ad = ads[0];
    ad.format = VK_FORMAT_R8G8B8A8_UNORM;
    ad.samples = VK_SAMPLE_COUNT_1_BIT;
    ad.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ad.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // TODO: (penguinliong) Support layout inference in the future.
    ad.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ad.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
  }
  // TODO: (penguinliong) Support input attachments.
  std::array<VkSubpassDescription, 1> sds {};
  {
    VkSubpassDescription& sd = sds[0];
    sd.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sd.inputAttachmentCount = 0;
    sd.pInputAttachments = nullptr;
    sd.colorAttachmentCount = (uint32_t)ars.size();
    sd.pColorAttachments = ars.data();
    sd.pResolveAttachments = nullptr;
    sd.pDepthStencilAttachment = nullptr;
    sd.preserveAttachmentCount = 0;
    sd.pPreserveAttachments = nullptr;
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

Task create_graph_task(
  const Context& ctxt,
  const GraphicsTaskConfig& cfg
) {
  std::vector<VkDescriptorPoolSize> desc_pool_sizes;
  VkDescriptorSetLayout desc_set_layout =
    _create_desc_set_layout(ctxt, cfg.rsc_tys, cfg.nrsc_ty, desc_pool_sizes);
  VkPipelineLayout pipe_layout = _create_pipe_layout(ctxt, desc_set_layout);
  VkShaderModule vert_shader_mod =
    _create_shader_mod(ctxt, cfg.vert_code, cfg.vert_code_size);
  VkShaderModule frag_shader_mod =
    _create_shader_mod(ctxt, cfg.frag_code, cfg.frag_code_size);
  VkRenderPass pass = _create_pass(ctxt);

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

  std::array<VkVertexInputBindingDescription, 1> vibds {
    VkVertexInputBindingDescription {
      0, 4 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX
    },
  };
  std::array<VkVertexInputAttributeDescription, 1> viads = {
    VkVertexInputAttributeDescription {
      0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 * sizeof(float)
    },
  };
  VkPipelineVertexInputStateCreateInfo pvisci {};
  pvisci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  pvisci.vertexBindingDescriptionCount = (uint32_t)vibds.size();
  pvisci.pVertexBindingDescriptions = vibds.data();
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
    liong::panic("unexpected topology (", cfg.topo, ")");
  }
  piasci.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport {};
  VkRect2D scissor {};
  VkPipelineViewportStateCreateInfo pvsci {};
  pvsci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  pvsci.viewportCount = 1;
  pvsci.pViewports = &viewport; // Dynamically specified.
  pvsci.scissorCount = 1;
  pvsci.pScissors = &scissor; // Dynamically specified.

  VkPipelineRasterizationStateCreateInfo prsci {};
  prsci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  prsci.cullMode = VK_CULL_MODE_NONE;
  prsci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  prsci.polygonMode = VK_POLYGON_MODE_FILL;

  VkPipelineMultisampleStateCreateInfo pmsci {};
  pmsci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  pmsci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo pdssci {};
  pdssci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  pdssci.depthTestEnable = VK_TRUE;
  pdssci.depthWriteEnable = VK_TRUE;
  pdssci.depthCompareOp = VK_COMPARE_OP_LESS;
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

  std::array<VkDynamicState, 2> dss {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo pdsci {};
  pdsci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  pdsci.dynamicStateCount = (uint32_t)dss.size();
  pdsci.pDynamicStates = dss.data();

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
  gpci.renderPass = pass;
  gpci.subpass = 0;

  VkPipeline pipe;
  VK_ASSERT << vkCreateGraphicsPipelines(ctxt.dev, VK_NULL_HANDLE, 1, &gpci,
    nullptr, &pipe);

  liong::log::info("created graphics task '", cfg.label, "'");
  return Task {
    &ctxt, desc_set_layout, pipe_layout, pipe, pass,
    { vert_shader_mod, frag_shader_mod }, std::move(desc_pool_sizes), cfg.label,
  };
}
void destroy_task(Task& task) {
  vkDestroyPipeline(task.ctxt->dev, task.pipe, nullptr);
  if (task.pass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(task.ctxt->dev, task.pass, nullptr);
  }
  for (const VkShaderModule& shader_mod : task.shader_mods) {
    vkDestroyShaderModule(task.ctxt->dev, shader_mod, nullptr);
  }
  task.shader_mods.clear();
  vkDestroyPipelineLayout(task.ctxt->dev, task.pipe_layout, nullptr);
  vkDestroyDescriptorSetLayout(task.ctxt->dev, task.desc_set_layout, nullptr);

  liong::log::info("destroyed task '", task.label, "'");
  task = {};
}



Framebuffer create_framebuf(
  const Context& ctxt,
  const Task& task,
  const Image& attm
) {
  VkFramebufferCreateInfo fci {};
  fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fci.renderPass = task.pass;
  fci.attachmentCount = 1;
  fci.pAttachments = &attm.img_view;
  fci.width = (uint32_t)attm.img_cfg.ncol;
  fci.height = (uint32_t)attm.img_cfg.nrow;
  fci.layers = 1;

  VkFramebuffer framebuf;
  VK_ASSERT << vkCreateFramebuffer(ctxt.dev, &fci, nullptr, &framebuf);

  VkRect2D viewport {};
  viewport.extent.width = (uint32_t)attm.img_cfg.ncol;
  viewport.extent.height = (uint32_t)attm.img_cfg.nrow;

  liong::log::info("created framebuffer");
  return { &ctxt, &task, &attm, std::move(viewport), framebuf };
}
void destroy_framebuf(Framebuffer& framebuf) {
  vkDestroyFramebuffer(framebuf.ctxt->dev, framebuf.framebuf, nullptr);
  framebuf.framebuf = VK_NULL_HANDLE;
  liong::log::info("destroyed framebuffer");
}



ResourcePool create_rsc_pool(
  const Context& ctxt,
  const Task& task
) {
  if (task.desc_pool_sizes.size() == 0) {
    liong::log::info("created resource pool with no entry");
    return ResourcePool { &ctxt, VK_NULL_HANDLE, VK_NULL_HANDLE };
  }

  VkDescriptorPoolCreateInfo dpci {};
  dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  dpci.poolSizeCount = static_cast<uint32_t>(task.desc_pool_sizes.size());
  dpci.pPoolSizes = task.desc_pool_sizes.data();
  dpci.maxSets = 1;

  VkDescriptorPool desc_pool;
  VK_ASSERT << vkCreateDescriptorPool(ctxt.dev, &dpci, nullptr, &desc_pool);

  VkDescriptorSetAllocateInfo dsai {};
  dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dsai.descriptorPool = desc_pool;
  dsai.descriptorSetCount = 1;
  dsai.pSetLayouts = &task.desc_set_layout;

  VkDescriptorSet desc_set;
  VK_ASSERT << vkAllocateDescriptorSets(ctxt.dev, &dsai, &desc_set);

  liong::log::info("created resource pool");
  return ResourcePool { &ctxt, desc_pool, desc_set };
}
void destroy_rsc_pool(ResourcePool& rsc_pool) {
  if (rsc_pool.desc_pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(rsc_pool.ctxt->dev, rsc_pool.desc_pool, nullptr);
    rsc_pool.desc_pool = VK_NULL_HANDLE;
  }
  liong::log::info("destroyed resource pool");
}
void bind_pool_rsc(
  ResourcePool& rsc_pool,
  uint32_t idx,
  const BufferView& buf_view
) {
  liong::assert(rsc_pool.desc_pool != VK_NULL_HANDLE,
    "cannot bind to empty resource pool");

  VkDescriptorBufferInfo dbi {};
  dbi.buffer = buf_view.buf->buf;
  dbi.offset = buf_view.offset;
  dbi.range = buf_view.size;

  VkWriteDescriptorSet write_desc_set {};
  write_desc_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_desc_set.dstSet = rsc_pool.desc_set;
  write_desc_set.dstBinding = idx;
  write_desc_set.dstArrayElement = 0;
  write_desc_set.descriptorCount = 1;
  if (buf_view.buf->buf_cfg.usage & L_BUFFER_USAGE_UNIFORM_BIT) {
    write_desc_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  }
  if (buf_view.buf->buf_cfg.usage & L_BUFFER_USAGE_STORAGE_BIT) {
    write_desc_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  }
  write_desc_set.pBufferInfo = &dbi;

  vkUpdateDescriptorSets(rsc_pool.ctxt->dev, 1, &write_desc_set, 0, nullptr);
  liong::log::info("bound pool resource #", idx, " to buffer '",
    buf_view.buf->buf_cfg.label, "'");
}
void bind_pool_rsc(
  ResourcePool& rsc_pool,
  uint32_t idx,
  const ImageView& img_view
) {
  liong::assert(rsc_pool.desc_pool != VK_NULL_HANDLE,
    "cannot bind to empty resource pool");

  VkDescriptorImageInfo dii {};
  dii.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  dii.imageView = img_view.img->img_view;

  VkWriteDescriptorSet write_desc_set {};
  write_desc_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_desc_set.dstSet = rsc_pool.desc_set;
  write_desc_set.dstBinding = idx;
  write_desc_set.dstArrayElement = 0;
  write_desc_set.descriptorCount = 1;
  switch (img_view.img->img_cfg.usage) {
  case L_IMAGE_USAGE_SAMPLED_BIT:
    write_desc_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    break;
  case L_IMAGE_USAGE_STORAGE_BIT:
    write_desc_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    break;
  case L_IMAGE_USAGE_ATTACHMENT_BIT:
    liong::panic("input attachment is not yet unsupported");
    break;
  default:
    liong::panic("unexpected image usage");
  }
  write_desc_set.pImageInfo = &dii;

  vkUpdateDescriptorSets(rsc_pool.ctxt->dev, 1, &write_desc_set, 0, nullptr);
  liong::log::info("bound pool resource #", idx, " to image '",
    img_view.img->img_cfg.label, "'");
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

VkCommandPool _create_cmd_pool(const Context& ctxt, uint32_t qfam_idx) {
  VkCommandPoolCreateInfo cpci {};
  cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cpci.queueFamilyIndex = qfam_idx;

  VkCommandPool cmd_pool;
  VK_ASSERT << vkCreateCommandPool(ctxt.dev, &cpci, nullptr, &cmd_pool);

  return cmd_pool;
}
std::array<VkCommandPool, 2> _collect_cmd_pools(
  const Context& ctxt
) {
  std::array<VkCommandPool, 2> cmd_pools {};
  for (auto i = 0; i < ctxt.submit_details.size(); ++i) {
    const auto& submit_detail = ctxt.submit_details[i];
    cmd_pools[i] = _create_cmd_pool(ctxt, submit_detail.qfam_idx);
  }

  return cmd_pools;
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
  std::array<VkCommandPool, 2> cmd_pools;
  std::vector<TransactionSubmitDetail> submit_details;
  std::vector<VkSemaphore> semas;
  VkFence fence;
  VkCommandBufferLevel level;
};
VkCommandBuffer _start_cmdbuf(
  TransactionLike& transact,
  SubmitType submit_ty
) {
  auto icmd_pool = transact.ctxt->get_queue_rsc_idx(submit_ty);
  auto queue = transact.ctxt->submit_details[icmd_pool].queue;
  auto qfam_idx = transact.ctxt->submit_details[icmd_pool].qfam_idx;
  auto cmd_pool = transact.cmd_pools[icmd_pool];
  auto cmdbuf = _alloc_cmdbuf(*transact.ctxt, cmd_pool, transact.level);

  VkCommandBufferInheritanceInfo cbii {};
  cbii.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

  VkCommandBufferBeginInfo cbbi {};
  cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  if (submit_ty == L_SUBMIT_TYPE_GRAPHICS) {
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
  }
  cbbi.pInheritanceInfo = &cbii;
  VK_ASSERT << vkBeginCommandBuffer(cmdbuf, &cbbi);

  TransactionSubmitDetail submit_detail;
  submit_detail.submit_ty = submit_ty;
  submit_detail.cmdbuf = cmdbuf;
  submit_detail.queue = queue;
  submit_detail.qfam_idx = qfam_idx;
  submit_detail.pass_detail.pass = VK_NULL_HANDLE;
  submit_detail.pass_detail.framebuf = VK_NULL_HANDLE;
  submit_detail.pass_detail.render_area = {};
  submit_detail.pass_detail.clear_value = {{{ 0.0f, 0.0f, 0.0f, 0.0f }}};

  transact.submit_details.emplace_back(std::move(submit_detail));
  return cmdbuf;
}
VkCommandBuffer _get_cmdbuf(
  TransactionLike& transact,
  SubmitType submit_ty
) {
  if (submit_ty == L_SUBMIT_TYPE_ANY) {
    liong::assert(!transact.submit_details.empty(),
      "cannot infer submit type for submit-type-independent command");
    submit_ty = transact.submit_details.back().submit_ty;
  }
  auto isubmit_detail = transact.ctxt->get_queue_rsc_idx(submit_ty);
  auto queue = transact.ctxt->submit_details[isubmit_detail].queue;
  auto qfam_idx = transact.ctxt->submit_details[isubmit_detail].qfam_idx;
  if (!transact.submit_details.empty()) {
    auto& last_submit = transact.submit_details.back();
    if (queue == last_submit.queue) {
      return last_submit.cmdbuf;
    } else {
      VK_ASSERT << vkEndCommandBuffer(last_submit.cmdbuf);
      if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
        VkPipelineStageFlags stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        VkSubmitInfo submit_info {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &last_submit.cmdbuf;
        submit_info.signalSemaphoreCount = 1;
        if (!transact.semas.empty()) {
          // Wait for the last submitted command buffer on the device side.
          submit_info.waitSemaphoreCount = 1;
          submit_info.pWaitSemaphores = &transact.semas.back();
          submit_info.pWaitDstStageMask = &stage_mask;
        }
        auto sema = _create_sema(*transact.ctxt);
        submit_info.pSignalSemaphores = &sema;

        // Finish recording and submit the command buffer to the device.
        VK_ASSERT <<
          vkQueueSubmit(last_submit.queue, 1, &submit_info, VK_NULL_HANDLE);

        // Don't push before queue submit, or the address might change.
        transact.semas.push_back(sema);
      }
    }
  }
  return _start_cmdbuf(transact, submit_ty);
}
void _seal_transact(TransactionLike& transact) {
  const auto& last_submit = transact.submit_details.back();
  VK_ASSERT << vkEndCommandBuffer(last_submit.cmdbuf);
  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    VkPipelineStageFlags stage_mask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

    VkSubmitInfo submit_info {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &last_submit.cmdbuf;
    if (!transact.semas.empty()) {
      // Wait for the last submitted command buffer on the device side.
      submit_info.waitSemaphoreCount = 1;
      submit_info.pWaitSemaphores = &transact.semas.back();
      submit_info.pWaitDstStageMask = &stage_mask;
    }

    // Finish recording and submit the command buffer to the device. The fence
    // is created externally.
    VK_ASSERT <<
      vkQueueSubmit(last_submit.queue, 1, &submit_info, transact.fence);
  }
}

void _record_cmd_set_submit_ty(
  TransactionLike& transact,
  const Command& cmd
) {
  SubmitType submit_ty = cmd.cmd_set_submit_ty.submit_ty;
  _get_cmdbuf(transact, submit_ty);
  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    liong::log::info("submit type is set");
  }
}
void _record_cmd_inline_transact(
  TransactionLike& transact,
  const Command& cmd
) {
  liong::assert(transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  const auto& in = cmd.cmd_inline_transact;
  const auto& subtransact = *in.transact;

  for (auto i = 0; i < subtransact.submit_details.size(); ++i) {
    const auto& submit_detail = subtransact.submit_details[i];
    auto cmdbuf = _get_cmdbuf(transact, submit_detail.submit_ty);

    if (submit_detail.submit_ty == L_SUBMIT_TYPE_GRAPHICS) {
      VkRenderPassBeginInfo rpbi {};
      rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      rpbi.renderPass = submit_detail.pass_detail.pass;
      rpbi.framebuffer = submit_detail.pass_detail.framebuf;
      rpbi.renderArea.extent = submit_detail.pass_detail.render_area;
      rpbi.clearValueCount = 1;
      rpbi.pClearValues = &submit_detail.pass_detail.clear_value;
      vkCmdBeginRenderPass(cmdbuf, &rpbi,
        VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    }
    vkCmdExecuteCommands(cmdbuf, 1, &submit_detail.cmdbuf);
    if (submit_detail.submit_ty == L_SUBMIT_TYPE_GRAPHICS) {
      vkCmdEndRenderPass(cmdbuf);
    }
  }
  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    liong::log::info("scheduled inline transaction '",
      cmd.cmd_inline_transact.transact->label, "'");
  }
}

void _record_cmd_copy_buf2img(TransactionLike& transact, const Command& cmd) {
  const auto& in = cmd.cmd_copy_buf2img;
  const auto& src = in.src;
  const auto& dst = in.dst;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  VkBufferImageCopy bic {};
  bic.bufferOffset = src.offset;
  bic.bufferRowLength = 0;
  bic.bufferImageHeight = (uint32_t)dst.img->img_cfg.nrow;
  bic.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bic.imageSubresource.mipLevel = 0;
  bic.imageSubresource.baseArrayLayer = 0;
  bic.imageSubresource.layerCount = 1;
  bic.imageOffset.x = dst.col_offset;
  bic.imageOffset.y = dst.row_offset;
  bic.imageExtent.width = dst.ncol;
  bic.imageExtent.height = dst.nrow;
  bic.imageExtent.depth = 1;

  vkCmdCopyBufferToImage(cmdbuf, src.buf->buf, dst.img->img,
    VK_IMAGE_LAYOUT_GENERAL, 1, &bic);
  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    liong::log::info("scheduled copy from buffer '", src.buf->buf_cfg.label,
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
  bic.bufferImageHeight = static_cast<uint32_t>(src.img->img_cfg.nrow);
  bic.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bic.imageSubresource.mipLevel = 0;
  bic.imageSubresource.baseArrayLayer = 0;
  bic.imageSubresource.layerCount = 1;
  bic.imageOffset.x = src.col_offset;
  bic.imageOffset.y = src.row_offset;
  bic.imageExtent.width = src.ncol;
  bic.imageExtent.height = src.nrow;
  bic.imageExtent.depth = 1;

  vkCmdCopyImageToBuffer(cmdbuf, src.img->img, VK_IMAGE_LAYOUT_GENERAL,
    dst.buf->buf, 1, &bic);
  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    liong::log::info("scheduled copy from image '", src.img->img_cfg.label,
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
    liong::log::info("scheduled copy from buffer '", src.buf->buf_cfg.label,
      "' to buffer '", dst.buf->buf_cfg.label, "'");
  }
}
void _record_cmd_copy_img(TransactionLike& transact, const Command& cmd) {
  const auto& in = cmd.cmd_copy_img;
  const auto& src = in.src;
  const auto& dst = in.dst;
  assert(src.ncol == dst.ncol && src.nrow == dst.nrow,
    "image copy size mismatched");
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  VkImageCopy ic {};
  ic.srcOffset.x = src.col_offset;
  ic.srcOffset.y = src.row_offset;
  ic.dstOffset.x = dst.col_offset;
  ic.dstOffset.y = dst.row_offset;
  ic.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  ic.srcSubresource.baseArrayLayer = 0;
  ic.srcSubresource.layerCount = 1;
  ic.srcSubresource.mipLevel = 0;
  ic.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  ic.dstSubresource.baseArrayLayer = 0;
  ic.dstSubresource.layerCount = 1;
  ic.dstSubresource.mipLevel = 0;
  ic.extent.width = dst.ncol;
  ic.extent.height = dst.nrow;
  ic.extent.depth = 1;

  vkCmdCopyImage(cmdbuf, src.img->img, VK_IMAGE_LAYOUT_GENERAL,
    dst.img->img, VK_IMAGE_LAYOUT_GENERAL, 1, &ic);
  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    liong::log::info("scheduled copy from image '", src.img->img_cfg.label,
      "' to image '", dst.img->img_cfg.label, "'");
  }
}

void _record_cmd_dispatch(TransactionLike& transact, const Command& cmd) {
  const auto& in = cmd.cmd_dispatch;
  const auto& task = *in.task;
  const auto& rsc_pool = *in.rsc_pool;
  const auto& nworkgrp = in.nworkgrp;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_COMPUTE);

  //liong::log::warn("workgroup size is ignored; actual workgroup size is "
  //  "specified in shader");
  vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, task.pipe);
  if (rsc_pool.desc_set != VK_NULL_HANDLE) {
    vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE,
      task.pipe_layout, 0, 1, &rsc_pool.desc_set, 0, nullptr);
  }
  vkCmdDispatch(cmdbuf, nworkgrp.x, nworkgrp.y, nworkgrp.z);
  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    liong::log::info("scheduled compute task '", task.label, "' for execution");
  }
}


void _record_cmd_draw_common(TransactionLike& transact, const Command& cmd) {
  const auto& task = cmd.cmd_ty == L_COMMAND_TYPE_DRAW ?
    *cmd.cmd_draw.task : *cmd.cmd_draw_indexed.task;
  const auto& rsc_pool = cmd.cmd_ty == L_COMMAND_TYPE_DRAW ?
    *cmd.cmd_draw.rsc_pool : *cmd.cmd_draw_indexed.rsc_pool;
  const auto& framebuf = cmd.cmd_ty == L_COMMAND_TYPE_DRAW ?
    *cmd.cmd_draw.framebuf : *cmd.cmd_draw_indexed.framebuf;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_GRAPHICS);

  // TODO: (penguinliong) Move this to a specialized command.
  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    VkRenderPassBeginInfo rpbi {};
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass = task.pass;
    rpbi.framebuffer = framebuf.framebuf;
    rpbi.renderArea.extent = framebuf.viewport.extent;
    rpbi.clearValueCount = 1;
    rpbi.pClearValues = &framebuf.clear_value;

    vkCmdBeginRenderPass(cmdbuf, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
  } else {
    auto& last_submit = transact.submit_details.back();
    liong::assert(last_submit.pass_detail.pass == VK_NULL_HANDLE,
      "secondary command buffer can only contain one render pass");
    last_submit.pass_detail.pass = task.pass;
    last_submit.pass_detail.framebuf = framebuf.framebuf;
    last_submit.pass_detail.render_area = framebuf.viewport.extent;
    last_submit.pass_detail.clear_value = framebuf.clear_value;
  }

  vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, task.pipe);
  if (rsc_pool.desc_set != VK_NULL_HANDLE) {
    vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
      task.pipe_layout, 0, 1, &rsc_pool.desc_set, 0, nullptr);
  }
  VkRect2D scissor {};
  scissor.extent = framebuf.viewport.extent;
  vkCmdSetScissor(cmdbuf, 0, 1, &scissor);
  VkViewport viewport {};
  viewport.width = (float)framebuf.viewport.extent.width;
  viewport.height = (float)framebuf.viewport.extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(cmdbuf, 0, 1, &viewport);

  if (cmd.cmd_ty == L_COMMAND_TYPE_DRAW) {
    const auto& in = cmd.cmd_draw;

    VkDeviceSize offset = in.verts.offset;
    vkCmdBindVertexBuffers(cmdbuf, 0, 1, &in.verts.buf->buf, &offset);
    vkCmdDraw(cmdbuf, in.nvert, in.ninst, 0, 0);

    if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
      liong::log::info("scheduled graphics task '", in.task->label,
        "' for execution");
    }
  } else if (cmd.cmd_ty == L_COMMAND_TYPE_DRAW_INDEXED) {
    const auto& in = cmd.cmd_draw_indexed;

    VkDeviceSize offset = in.verts.offset;
    vkCmdBindVertexBuffers(cmdbuf, 0, 1, &in.verts.buf->buf, &offset);

    vkCmdBindIndexBuffer(cmdbuf, in.idxs.buf->buf, in.idxs.offset,
      VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(cmdbuf, in.nidx, in.ninst, 0, 0, 0);

    if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
      liong::log::info("scheduled graphics task '", in.task->label,
        "' for execution");
    }
  } else {
    liong::panic("unexpected command type ", cmd.cmd_ty);
  }

  // TODO: (penguinliong) Move this to a specialized command.
  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    vkCmdEndRenderPass(cmdbuf);
  }
}

void _record_cmd_write_timestamp(
  TransactionLike& transact,
  const Command& cmd
) {
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  auto query_pool = cmd.cmd_write_timestamp.timestamp->query_pool;
  vkCmdResetQueryPool(cmdbuf, query_pool, 0, 1);
  vkCmdWriteTimestamp(cmdbuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, query_pool, 0);

  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    liong::log::info("schedule timestamp write");
  }
}













void _make_buf_barrier_params(
  BufferUsage usage,
  MemoryAccess dev_access,
  VkAccessFlags& access,
  VkPipelineStageFlags stage
) {
  if (dev_access == L_MEMORY_ACCESS_NONE) { return; }

  if (usage == L_BUFFER_USAGE_STAGING_BIT) {
    if (dev_access == L_MEMORY_ACCESS_READ_ONLY) {
      // Transfer source.
      access = VK_ACCESS_TRANSFER_READ_BIT;
      stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (dev_access == L_MEMORY_ACCESS_WRITE_ONLY) {
      // Transfer destination.
      access = VK_ACCESS_TRANSFER_WRITE_BIT;
      stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else {
      return;
    }
  } else if (usage == L_BUFFER_USAGE_VERTEX_BIT) {
    if (dev_access == L_MEMORY_ACCESS_READ_ONLY) {
      access = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
      stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
    } else {
      return;
    }
  } else if (usage == L_BUFFER_USAGE_INDEX_BIT) {
    if (dev_access == L_MEMORY_ACCESS_READ_ONLY) {
      access = VK_ACCESS_INDEX_READ_BIT;
      stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
    } else {
      return;
    }
  } else if (usage == L_BUFFER_USAGE_UNIFORM_BIT) {
    if (dev_access == L_MEMORY_ACCESS_READ_ONLY) {
      access = VK_ACCESS_UNIFORM_READ_BIT;
      stage =
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    } else {
      return;
    }
  } else if (usage == L_BUFFER_USAGE_STORAGE_BIT) {
    if (dev_access == L_MEMORY_ACCESS_READ_ONLY) {
      access = VK_ACCESS_SHADER_READ_BIT;
      stage =
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    } else if (dev_access == L_MEMORY_ACCESS_WRITE_BIT) {
      access = VK_ACCESS_SHADER_WRITE_BIT;
      stage =
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    } else {
      access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      stage =
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
  }
}

void _make_img_barrier_src_params(
  ImageUsage usage,
  MemoryAccess dev_access,
  VkAccessFlags& access,
  VkPipelineStageFlags& stage,
  VkImageLayout& layout
) {
  if (dev_access == L_MEMORY_ACCESS_NONE) { return; }

  if (usage == L_IMAGE_USAGE_STAGING_BIT) {
    if (dev_access == L_MEMORY_ACCESS_READ_ONLY) {
      // Transfer source.
      access = VK_ACCESS_TRANSFER_READ_BIT;
      stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    } else if (dev_access == L_MEMORY_ACCESS_WRITE_ONLY) {
      // Transfer destination.
      access = VK_ACCESS_TRANSFER_WRITE_BIT;
      stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    } else {
      return;
    }
  } else if (usage == L_IMAGE_USAGE_ATTACHMENT_BIT) {
    if (dev_access == L_MEMORY_ACCESS_READ_ONLY) {
      // Input attachment.
      access = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
      stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    } else {
      // Fragment output.
      access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Note: This is different.
      stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
  } else if (usage == L_IMAGE_USAGE_SAMPLED_BIT) {
    if (dev_access == L_MEMORY_ACCESS_READ_ONLY) {
      // Sampled image.
      access = VK_ACCESS_SHADER_READ_BIT;
      stage =
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    } else {
      return;
    }
  } else if (usage == L_IMAGE_USAGE_STORAGE_BIT) {
    if (dev_access == L_MEMORY_ACCESS_READ_ONLY) {
      // Read-only storage image.
      access = VK_ACCESS_SHADER_READ_BIT;
      stage =
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      layout = VK_IMAGE_LAYOUT_GENERAL;
    } else if (dev_access == L_MEMORY_ACCESS_WRITE_ONLY) {
      // Write-only storage image.
      access = VK_ACCESS_SHADER_WRITE_BIT;
      stage =
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      layout = VK_IMAGE_LAYOUT_GENERAL;
    } else {
      // Read-write storage image.
      access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      stage =
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      layout = VK_IMAGE_LAYOUT_GENERAL;
    }
  }
}
void _make_img_barrier_dst_params(
  ImageUsage usage,
  MemoryAccess dev_access,
  VkAccessFlags& access,
  VkPipelineStageFlags& stage,
  VkImageLayout& layout
) {
  if (dev_access == L_MEMORY_ACCESS_NONE) { return; }

  if (usage == L_IMAGE_USAGE_STAGING_BIT) {
    if (dev_access == L_MEMORY_ACCESS_READ_ONLY) {
      // Transfer source.
      access = VK_ACCESS_TRANSFER_READ_BIT;
      stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    } else if (dev_access == L_MEMORY_ACCESS_WRITE_ONLY) {
      // Transfer destination.
      access = VK_ACCESS_TRANSFER_WRITE_BIT;
      stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    } else {
      return;
    }
  } else if (usage == L_IMAGE_USAGE_ATTACHMENT_BIT) {
    if (dev_access == L_MEMORY_ACCESS_READ_ONLY) {
      // Input attachment.
      access = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
      stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    } else {
      // Fragment output.
      access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
      stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
  } else if (usage == L_IMAGE_USAGE_SAMPLED_BIT) {
    if (dev_access == L_MEMORY_ACCESS_READ_ONLY) {
      // Sampled image.
      access = VK_ACCESS_SHADER_READ_BIT;
      stage =
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    } else {
      return;
    }
  } else if (usage == L_IMAGE_USAGE_STORAGE_BIT) {
    if (dev_access == L_MEMORY_ACCESS_READ_ONLY) {
      // Read-only storage image.
      access = VK_ACCESS_SHADER_READ_BIT;
      stage =
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      layout = VK_IMAGE_LAYOUT_GENERAL;
    } else if (dev_access == L_MEMORY_ACCESS_WRITE_ONLY) {
      // Write-only storage image.
      access = VK_ACCESS_SHADER_WRITE_BIT;
      stage =
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      layout = VK_IMAGE_LAYOUT_GENERAL;
    } else {
      // Read-write storage image.
      access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      stage =
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      layout = VK_IMAGE_LAYOUT_GENERAL;
    }
  }
}
void _record_cmd_buf_barrier(
  TransactionLike& transact,
  const Command& cmd
) {
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  BufferUsage src_usage = cmd.cmd_buf_barrier.src_usage;
  BufferUsage dst_usage = cmd.cmd_buf_barrier.dst_usage;
  MemoryAccess src_dev_access = cmd.cmd_buf_barrier.src_dev_access;
  MemoryAccess dst_dev_access = cmd.cmd_buf_barrier.dst_dev_access;

  VkAccessFlags src_access = 0;
  VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  _make_buf_barrier_params(src_usage, src_dev_access, src_access, src_stage);

  VkAccessFlags dst_access = 0;
  VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  _make_buf_barrier_params(dst_usage, dst_dev_access, dst_access, dst_stage);

  VkBufferMemoryBarrier bmb {};
  bmb.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bmb.buffer = cmd.cmd_buf_barrier.buf->buf;
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
    liong::log::info("scheduled buffer barrier");
  }
}
void _record_cmd_img_barrier(
  TransactionLike& transact,
  const Command& cmd
) {
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  ImageUsage src_usage = cmd.cmd_img_barrier.src_usage;
  ImageUsage dst_usage = cmd.cmd_img_barrier.dst_usage;
  MemoryAccess src_dev_access = cmd.cmd_img_barrier.src_dev_access;
  MemoryAccess dst_dev_access = cmd.cmd_img_barrier.dst_dev_access;

  VkAccessFlags src_access = 0;
  VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  VkImageLayout src_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  _make_img_barrier_src_params(src_usage, src_dev_access,
    src_access, src_stage, src_layout);

  VkAccessFlags dst_access = 0;
  VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkImageLayout dst_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  _make_img_barrier_dst_params(dst_usage, dst_dev_access,
    dst_access, dst_stage, dst_layout);

  VkImageMemoryBarrier imb {};
  imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imb.image = cmd.cmd_img_barrier.img->img;
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
    liong::log::info("scheduled image barrier");
  }
}

// Returns whether the submit queue to submit has changed.
void _record_cmd(TransactionLike& transact, const Command& cmd) {
  switch (cmd.cmd_ty) {
  case L_COMMAND_TYPE_SET_SUBMIT_TYPE:
    _record_cmd_set_submit_ty(transact, cmd);
    break;
  case L_COMMAND_TYPE_INLINE_TRANSACTION:
    assert(transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      "nested inline transaction is not allowed");
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
  case L_COMMAND_TYPE_DISPATCH:
    _record_cmd_dispatch(transact, cmd);
    break;
  case L_COMMAND_TYPE_DRAW:
    _record_cmd_draw_common(transact, cmd);
    break;
  case L_COMMAND_TYPE_DRAW_INDEXED:
    _record_cmd_draw_common(transact, cmd);
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
  default:
    liong::log::warn("ignored unknown command: ", cmd.cmd_ty);
    break;
  }
}



CommandDrain create_cmd_drain(const Context& ctxt) {
  auto fence = _create_fence(ctxt);
  auto cmd_pools = _collect_cmd_pools(ctxt);
  liong::log::info("created command drain");
  return CommandDrain { &ctxt, std::move(cmd_pools), {}, fence, {}, {} };
}
void destroy_cmd_drain(CommandDrain& cmd_drain) {
  for (auto cmd_pool : cmd_drain.cmd_pools) {
    if (cmd_pool == VK_NULL_HANDLE) { break; }
    vkDestroyCommandPool(cmd_drain.ctxt->dev, cmd_pool, nullptr);
  }
  vkDestroyFence(cmd_drain.ctxt->dev, cmd_drain.fence, nullptr);
  cmd_drain = {};
  liong::log::info("destroyed command drain");
}
void submit_cmds(
  CommandDrain& cmd_drain,
  const Command* cmds,
  size_t ncmd
) {
  auto tic = std::chrono::high_resolution_clock::now();
  TransactionLike transact;
  transact.ctxt = cmd_drain.ctxt;
  transact.cmd_pools = std::move(cmd_drain.cmd_pools);
  transact.fence = cmd_drain.fence;
  transact.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  for (auto i = 0; i < ncmd; ++i) {
    liong::log::debug("recording ", i, "th command");
    _record_cmd(transact, cmds[i]);
  }
  _seal_transact(transact);
  cmd_drain.semas = std::move(transact.semas);
  cmd_drain.cmd_pools = std::move(transact.cmd_pools);
  auto toc = std::chrono::high_resolution_clock::now();

  cmd_drain.submit_since = toc;
  std::chrono::duration<double, std::micro> drecord = toc - tic;
  liong::log::info("submitted transaction for execution, command recording "
    "took ", drecord.count(), "us");
}
void _reset_cmd_drain(CommandDrain& cmd_drain) {
  for (auto sema : cmd_drain.semas) {
    vkDestroySemaphore(cmd_drain.ctxt->dev, sema, nullptr);
  }
  cmd_drain.semas.clear();
  for (auto cmd_pool : cmd_drain.cmd_pools) {
    if (cmd_pool == VK_NULL_HANDLE) { break; }
    VK_ASSERT << vkResetCommandPool(cmd_drain.ctxt->dev, cmd_pool, 0);
  }
  VK_ASSERT << vkResetFences(cmd_drain.ctxt->dev, 1, &cmd_drain.fence);
}
void wait_cmd_drain(CommandDrain& cmd_drain) {
  const uint32_t SPIN_INTERVAL = 3000;
  auto tic = std::chrono::high_resolution_clock::now();
  for (VkResult err;;) {
    err = vkWaitForFences(cmd_drain.ctxt->dev, 1, &cmd_drain.fence, VK_TRUE,
        SPIN_INTERVAL);
    if (err == VK_TIMEOUT) {
      // liong::log::warn("timeout after 3000ns");
    } else {
      VK_ASSERT << err;
      break;
    }
  }
  auto toc = std::chrono::high_resolution_clock::now();
  _reset_cmd_drain(cmd_drain);
  auto toc2 = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double, std::micro> dwait = toc - tic;
  std::chrono::duration<double, std::micro> dsubmit =
    toc2 - cmd_drain.submit_since;
  std::chrono::duration<double, std::micro> dreset =
    toc2 - toc;

  liong::log::info("command drain returned after ", dwait.count(), "us since "
    "the wait started, ", dsubmit.count(), "us since submission "
    "(spin interval = ", SPIN_INTERVAL / 1000.0, "us; resource recycling took ",
    dreset.count(), "us)");

}



Transaction create_transact(
  const std::string& label,
  const Context& ctxt,
  const Command* cmds,
  size_t ncmd
) {
  TransactionLike transact;
  transact.ctxt = &ctxt;
  transact.cmd_pools = _collect_cmd_pools(ctxt);
  transact.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  for (auto i = 0; i < ncmd; ++i) {
    _record_cmd(transact, cmds[i]);
  }
  _seal_transact(transact);

  liong::log::info("created transaction");
  return Transaction { label, &ctxt, std::move(transact.cmd_pools),
    std::move(transact.submit_details) };
}
void destroy_transact(Transaction& transact) {
  for (auto cmd_pool : transact.cmd_pools) {
    if (cmd_pool == VK_NULL_HANDLE) { break; }
    vkDestroyCommandPool(transact.ctxt->dev, cmd_pool, nullptr);
  }
  transact = {};
  liong::log::info("destroyed transaction");
}



Timestamp create_timestamp(const Context& ctxt) {
  VkQueryPoolCreateInfo qpci {};
  qpci.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  qpci.queryCount = 1;
  qpci.queryType = VK_QUERY_TYPE_TIMESTAMP;

  VkQueryPool query_pool;
  VK_ASSERT << vkCreateQueryPool(ctxt.dev, &qpci, nullptr, &query_pool);

  liong::log::info("created timestamp");
  return Timestamp { &ctxt, query_pool };
}
void destroy_timestamp(Timestamp& timestamp) {
  vkDestroyQueryPool(timestamp.ctxt->dev, timestamp.query_pool, nullptr);
  timestamp = {};
  liong::log::info("destroyed timestamp");
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
