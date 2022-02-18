#if GFT_WITH_VULKAN

#include <map>
#include <set>
#include <algorithm>
#define VMA_IMPLEMENTATION
#include "gft/vk.hpp" // Defines `HAL_IMPL_NAMESPACE`.
#include "gft/assert.hpp"
#include "gft/util.hpp"
#include "gft/log.hpp"
#include "gft/hal/scoped-hal-impl.hpp" // `HAL_IMPL_AMESPACE` used here.

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
  };
  std::vector<SubmitTypeQueueRequirement> submit_ty_reqs {
    SubmitTypeQueueRequirement {
      L_SUBMIT_TYPE_ANY,
      ~VkQueueFlags(0),
      "ANY",
    },
    SubmitTypeQueueRequirement {
      L_SUBMIT_TYPE_GRAPHICS,
      VK_QUEUE_GRAPHICS_BIT,
      "GRAPHICS",
    },
    SubmitTypeQueueRequirement {
      L_SUBMIT_TYPE_COMPUTE,
      VK_QUEUE_COMPUTE_BIT,
      "COMPUTE",
    },
    SubmitTypeQueueRequirement {
      L_SUBMIT_TYPE_TRANSFER,
      VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT,
      "TRANSFER",
    },
  };

  std::map<SubmitType, uint32_t> queue_allocs;
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
        if ((req_queue_flags & qfam_trait.queue_flags) != 0) {
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
  allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;
  allocatorInfo.physicalDevice = physdev;
  allocatorInfo.device = dev;
  allocatorInfo.instance = inst;

  VmaAllocator allocator;
  VK_ASSERT << vmaCreateAllocator(&allocatorInfo, &allocator);

  log::debug("created vulkan context '", cfg.label, "' on device #",
    cfg.dev_idx, ": ", physdev_descs[cfg.dev_idx]);
  return Context {
    dev, physdev, std::move(physdev_prop), std::move(submit_details),
    img_samplers, depth_img_samplers, allocator, cfg
  };
}
Context create_ctxt(uint32_t dev_idx, const std::string& label) {
  ContextConfig cfg {};
  cfg.label = label;
  cfg.dev_idx = dev_idx;
  return create_ctxt(cfg);
}
void destroy_ctxt(Context& ctxt) {
  if (ctxt.dev != VK_NULL_HANDLE) {
    for (const auto& samp : ctxt.img_samplers) {
      vkDestroySampler(ctxt.dev, samp.second, nullptr);
    }
    for (const auto& samp : ctxt.depth_img_samplers) {
      vkDestroySampler(ctxt.dev, samp.second, nullptr);
    }
    vmaDestroyAllocator(ctxt.allocator);
    vkDestroyDevice(ctxt.dev, nullptr);
    log::debug("destroyed vulkan context '", ctxt.ctxt_cfg.label, "'");
  }
  ctxt = {};
}
const ContextConfig& get_ctxt_cfg(const Context& ctxt) {
  return ctxt.ctxt_cfg;
}


constexpr VmaMemoryUsage _host_access2vma_usage(MemoryAccess host_access) {
  if (host_access == 0) {
    return VMA_MEMORY_USAGE_GPU_ONLY;
  } else if (host_access == L_MEMORY_ACCESS_READ_BIT) {
    return VMA_MEMORY_USAGE_GPU_TO_CPU;
  } else if (host_access == L_MEMORY_ACCESS_WRITE_BIT) {
    return VMA_MEMORY_USAGE_CPU_TO_GPU;
  } else {
    return VMA_MEMORY_USAGE_CPU_ONLY;
  }
}
void _alloc_buf(
  const Context& ctxt,
  const VkBufferCreateInfo& bci,
  VmaMemoryUsage vma_usage,
  VkBuffer& buf,
  VmaAllocation& alloc
) {
  VmaAllocationCreateInfo aci {};
  aci.usage = vma_usage;

  VK_ASSERT <<
    vmaCreateBuffer(ctxt.allocator, &bci, &aci, &buf, &alloc, nullptr);
}
Buffer create_buf(const Context& ctxt, const BufferConfig& buf_cfg) {
  VkBufferCreateInfo bci {};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (buf_cfg.usage & L_BUFFER_USAGE_TRANSFER_SRC_BIT) {
    bci.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  }
  if (buf_cfg.usage & L_BUFFER_USAGE_TRANSFER_DST_BIT) {
    bci.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
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

  VmaMemoryUsage vma_usage = _host_access2vma_usage(buf_cfg.host_access);
  VkBuffer buf;
  VmaAllocation alloc;
  _alloc_buf(ctxt, bci, vma_usage, buf, alloc);

  BufferDynamicDetail dyn_detail {};
  dyn_detail.access = 0;
  dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;

  log::debug("created buffer '", buf_cfg.label, "'");
  return Buffer { &ctxt, alloc, buf, buf_cfg, std::move(dyn_detail) };
}
void destroy_buf(Buffer& buf) {
  if (buf.buf != VK_NULL_HANDLE) {
    vmaDestroyBuffer(buf.ctxt->allocator, buf.buf, buf.alloc);
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
  assert(map_access != 0, "memory map access must be read, write or both");

  VK_ASSERT << vmaMapMemory(buf.buf->ctxt->allocator, buf.buf->alloc, &mapped);

  auto& dyn_detail = (BufferDynamicDetail&)buf.buf->dyn_detail;
  dyn_detail.access = map_access == L_MEMORY_ACCESS_READ_BIT ?
    VK_ACCESS_HOST_READ_BIT : VK_ACCESS_HOST_WRITE_BIT;
  dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;

  log::debug("mapped buffer '", buf.buf->buf_cfg.label, "' from ", buf.offset,
    " to ", buf.offset + buf.size);
}
void unmap_buf_mem(
  const BufferView& buf,
  void* mapped
) {
  vmaUnmapMemory(buf.buf->ctxt->allocator, buf.buf->alloc);
  log::debug("unmapped buffer '", buf.buf->buf_cfg.label, "'");
}



VkFormat _make_img_fmt(fmt::Format fmt) {
  using namespace fmt;
  switch (fmt) {
  case L_FORMAT_R8G8B8A8_UNORM_PACK32: return VK_FORMAT_R8G8B8A8_UNORM;
  case L_FORMAT_R16G16B16A16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
  case L_FORMAT_R32_SFLOAT: return VK_FORMAT_R32_SFLOAT;
  case L_FORMAT_R32G32_SFLOAT: return VK_FORMAT_R32G32_SFLOAT;
  case L_FORMAT_R32G32B32A32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
  default: panic("unrecognized pixel format");
  }
  return VK_FORMAT_UNDEFINED;
}
void _alloc_img(
  const Context& ctxt,
  const VkImageCreateInfo& ici,
  bool is_tile_mem,
  VmaMemoryUsage vma_usage,
  VkImage& img,
  VmaAllocation& alloc
) {
  VmaAllocationCreateInfo aci {};
  VkResult res = VK_ERROR_OUT_OF_DEVICE_MEMORY;
  if (is_tile_mem) {
    aci.usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
    res = vmaCreateImage(ctxt.allocator, &ici, &aci, &img, &alloc, nullptr);
  }
  if (res != VK_SUCCESS) {
    if (is_tile_mem) {
      log::warn("tile-memory is unsupported, fall back to regular memory");
    }
    aci.usage = vma_usage;
    VK_ASSERT <<
      vmaCreateImage(ctxt.allocator, &ici, &aci, &img, &alloc, nullptr);
  }
}
Image create_img(const Context& ctxt, const ImageConfig& img_cfg) {
  VkFormat fmt = _make_img_fmt(img_cfg.fmt);
  VkImageUsageFlags usage = 0;
  SubmitType init_submit_ty = L_SUBMIT_TYPE_ANY;
  bool is_staging_img = false;

  if (img_cfg.usage & L_IMAGE_USAGE_TRANSFER_SRC_BIT) {
    usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    init_submit_ty = L_SUBMIT_TYPE_ANY;
  }
  if (img_cfg.usage & L_IMAGE_USAGE_TRANSFER_DST_BIT) {
    usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    init_submit_ty = L_SUBMIT_TYPE_ANY;
  }
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
  // KEEP THIS AFTER ANY SUBMIT TYPES.
  if (img_cfg.usage & L_IMAGE_USAGE_ATTACHMENT_BIT) {
    usage |=
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
      VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    init_submit_ty = L_SUBMIT_TYPE_GRAPHICS;
  }
  if (img_cfg.usage & L_IMAGE_USAGE_SUBPASS_DATA_BIT) {
    usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    init_submit_ty = L_SUBMIT_TYPE_GRAPHICS;
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

  bool is_tile_mem = img_cfg.usage & L_IMAGE_USAGE_TILE_MEMORY_BIT;
  VmaMemoryUsage vma_usage = _host_access2vma_usage(0);
  VkImage img;
  VmaAllocation alloc;
  _alloc_img(ctxt, ici, is_tile_mem, vma_usage, img, alloc);

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

  ImageDynamicDetail dyn_detail {};
  dyn_detail.layout = layout;
  dyn_detail.access = 0;
  dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;

  log::debug("created image '", img_cfg.label, "'");
  uint32_t qfam_idx = ctxt.submit_details.at(init_submit_ty).qfam_idx;
  return Image {
    &ctxt, alloc, img, img_view, img_cfg, is_staging_img, std::move(dyn_detail)
  };
}
void destroy_img(Image& img) {
  if (img.img != VK_NULL_HANDLE) {
    vkDestroyImageView(img.ctxt->dev, img.img_view, nullptr);
    vmaDestroyImage(img.ctxt->allocator, img.img, img.alloc);

    log::debug("destroyed image '", img.img_cfg.label, "'");
    img = {};
  }
}
const ImageConfig& get_img_cfg(const Image& img) {
  return img.img_cfg;
}



VkFormat _make_depth_fmt(fmt::DepthFormat fmt) {
  using namespace fmt;
  switch (fmt) {
  case L_DEPTH_FORMAT_D16_UNORM: return VK_FORMAT_D16_UNORM;
  case L_DEPTH_FORMAT_D32_SFLOAT: return VK_FORMAT_D32_SFLOAT;
  default: panic("unsupported depth format");
  }
  return VK_FORMAT_UNDEFINED;
}
DepthImage create_depth_img(
  const Context& ctxt,
  const DepthImageConfig& depth_img_cfg
) {
  VkFormat fmt = _make_depth_fmt(depth_img_cfg.fmt);
  VkImageUsageFlags usage = 0;
  SubmitType init_submit_ty = L_SUBMIT_TYPE_ANY;

  if (depth_img_cfg.usage & L_DEPTH_IMAGE_USAGE_SAMPLED_BIT) {
    usage |=
      VK_IMAGE_USAGE_SAMPLED_BIT |
      VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    init_submit_ty = L_SUBMIT_TYPE_ANY;
  }
  if (depth_img_cfg.usage & L_DEPTH_IMAGE_USAGE_ATTACHMENT_BIT) {
    usage |=
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    init_submit_ty = L_SUBMIT_TYPE_GRAPHICS;
  }
  // KEEP THIS AFTER ANY SUBMIT TYPES.
  if (depth_img_cfg.usage & L_DEPTH_IMAGE_USAGE_SUBPASS_DATA_BIT) {
    usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    init_submit_ty = L_SUBMIT_TYPE_GRAPHICS;
  }

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

  bool is_tile_mem = depth_img_cfg.usage & L_DEPTH_IMAGE_USAGE_TILE_MEMORY_BIT;
  VkImage img;
  VmaAllocation alloc;
  _alloc_img(ctxt, ici, is_tile_mem, VMA_MEMORY_USAGE_GPU_ONLY, img, alloc);

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
    (fmt::get_fmt_depth_nbit(depth_img_cfg.fmt) > 0 ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
    (fmt::get_fmt_stencil_nbit(depth_img_cfg.fmt) > 0 ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
  ivci.subresourceRange.baseArrayLayer = 0;
  ivci.subresourceRange.layerCount = 1;
  ivci.subresourceRange.baseMipLevel = 0;
  ivci.subresourceRange.levelCount = 1;

  VkImageView img_view;
  VK_ASSERT << vkCreateImageView(ctxt.dev, &ivci, nullptr, &img_view);

  DepthImageDynamicDetail dyn_detail {};
  dyn_detail.layout = layout;
  dyn_detail.access = 0;
  dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;

  log::debug("created depth image '", depth_img_cfg.label, "'");
  return DepthImage {
    &ctxt, alloc, img, img_view, depth_img_cfg, std::move(dyn_detail)
  };
}
void destroy_depth_img(DepthImage& depth_img) {
  if (depth_img.img) {
    vkDestroyImageView(depth_img.ctxt->dev, depth_img.img_view, nullptr);
    vmaDestroyImage(depth_img.ctxt->allocator, depth_img.img, depth_img.alloc);

    log::debug("destroyed depth image '", depth_img.depth_img_cfg.label, "'");
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
  assert(map_access != 0, "memory map access must be read, write or both");

  VkImageSubresource is {};
  is.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  is.arrayLayer = 0;
  is.mipLevel = 0;

  VkSubresourceLayout sl {};
  vkGetImageSubresourceLayout(img.img->ctxt->dev, img.img->img, &is, &sl);
  size_t offset = sl.offset;
  size_t size = sl.size;

  VK_ASSERT << vmaMapMemory(img.img->ctxt->allocator, img.img->alloc, &mapped);
  row_pitch = sl.rowPitch;

  auto& dyn_detail = (ImageDynamicDetail&)(img.img->dyn_detail);
  assert(dyn_detail.layout == VK_IMAGE_LAYOUT_PREINITIALIZED,
    "linear image cannot be initialized after other use");
  dyn_detail.access = map_access == L_MEMORY_ACCESS_READ_BIT ?
    VK_ACCESS_HOST_READ_BIT : VK_ACCESS_HOST_WRITE_BIT;
  dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;

  log::debug("mapped image '", img.img->img_cfg.label, "' from (", img.x_offset,
    ", ", img.y_offset, ") to (", img.x_offset + img.width, ", ",
    img.y_offset + img.height, ")");
}
void unmap_img_mem(
  const ImageView& img,
  void* mapped
) {
  vmaUnmapMemory(img.img->ctxt->allocator, img.img->alloc);
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
  assert(cfg.workgrp_size.x * cfg.workgrp_size.y * cfg.workgrp_size.z != 0,
    "workgroup size cannot be zero");
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
    cfg.label, L_SUBMIT_TYPE_COMPUTE, &ctxt, nullptr, desc_set_layout,
    pipe_layout, pipe, cfg.rsc_tys, { shader_mod }, std::move(desc_pool_sizes)
  };
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
      ar.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      ad.format = _make_img_fmt(attm_cfg.color_fmt);
      ad.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      ad.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      sar.color_attm_ref.emplace_back(std::move(ar));
      break;
    }
    case L_ATTACHMENT_TYPE_DEPTH:
    {
      assert(!sar.has_depth_attm, "subpass can only have one depth attachment");
      ar.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      ad.format = _make_depth_fmt(attm_cfg.depth_fmt);
      ad.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      ad.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      sar.has_depth_attm = true;
      sar.depth_attm_ref = std::move(ar);
      break;
    }
    default: unreachable();
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

RenderPass create_pass(const Context& ctxt, const RenderPassConfig& cfg) {
  VkRenderPass pass = _create_pass(ctxt, cfg.attm_cfgs);

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

  log::debug("created render pass '", cfg.label, "'");
  return RenderPass { &ctxt, viewport, pass, cfg, clear_values };
}
void destroy_pass(RenderPass& pass) {
  vkDestroyRenderPass(pass.ctxt->dev, pass.pass, nullptr);
  log::debug("destroyed render pass '", pass.pass_cfg.label, "'");
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
    size_t fmt_size = fmt::get_fmt_size(vert_input.fmt);

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
    cfg.label, L_SUBMIT_TYPE_GRAPHICS, &ctxt, &pass, desc_set_layout,
    pipe_layout, pipe, cfg.rsc_tys, { vert_shader_mod, frag_shader_mod },
    std::move(desc_pool_sizes)
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
  dbis.reserve(rsc_views.size());
  diis.reserve(rsc_views.size());
  wdss.reserve(rsc_views.size());

  auto push_dbi = [&](const ResourceView& rsc_view) {
    assert(rsc_view.rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER);
    const BufferView& buf_view = rsc_view.buf_view;

    VkDescriptorBufferInfo dbi {};
    dbi.buffer = buf_view.buf->buf;
    dbi.offset = buf_view.offset;
    dbi.range = buf_view.size;
    dbis.emplace_back(std::move(dbi));

    log::debug("bound pool resource #", wdss.size(), " to buffer '",
      buf_view.buf->buf_cfg.label, "'");

    return &dbis.back();
  };
  auto push_dii = [&](const ResourceView& rsc_view, VkImageLayout layout) {
    VkDescriptorImageInfo dii {};
    if (rsc_view.rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE) {
      const ImageView& img_view = rsc_view.img_view;
      dii.sampler = ctxt.img_samplers.at(img_view.sampler);
      dii.imageView = img_view.img->img_view;
      dii.imageLayout = layout;
      diis.emplace_back(std::move(dii));

      log::debug("bound pool resource #", wdss.size(), " to image '",
        img_view.img->img_cfg.label, "'");
    } else if (rsc_view.rsc_view_ty == L_RESOURCE_VIEW_TYPE_DEPTH_IMAGE) {
      const DepthImageView& depth_img_view = rsc_view.depth_img_view;

      dii.sampler = ctxt.depth_img_samplers.at(depth_img_view.sampler);
      dii.imageView = depth_img_view.depth_img->img_view;
      dii.imageLayout = layout;
      diis.emplace_back(std::move(dii));

      log::debug("bound pool resource #", wdss.size(), " to depth image '",
        depth_img_view.depth_img->depth_img_cfg.label, "'");
    } else {
      panic();
    }

    return &diis.back();
  };

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
      wds.pBufferInfo = push_dbi(rsc_view);
      wds.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      break;
    case L_RESOURCE_TYPE_STORAGE_BUFFER:
      wds.pBufferInfo = push_dbi(rsc_view);
      wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      break;
    case L_RESOURCE_TYPE_SAMPLED_IMAGE:
      wds.pImageInfo = push_dii(rsc_view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
      wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      break;
    case L_RESOURCE_TYPE_STORAGE_IMAGE:
      wds.pImageInfo = push_dii(rsc_view,
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
VkFramebuffer _create_framebuf(
  const RenderPass& pass,
  const std::vector<ResourceView>& attms
) {
  const RenderPassConfig& pass_cfg = pass.pass_cfg;

  assert(pass_cfg.attm_cfgs.size() == attms.size(),
    "number of provided attachments mismatches render pass requirement");
  std::vector<VkImageView> attm_img_views;

  uint32_t width = pass_cfg.width;
  uint32_t height = pass_cfg.height;

  for (size_t i = 0; i < attms.size(); ++i) {
    const AttachmentConfig& attm_cfg = pass_cfg.attm_cfgs[i];
    const ResourceView& attm = attms[i];

    switch (attm_cfg.attm_ty) {
    case L_ATTACHMENT_TYPE_COLOR:
    {
      const Image& img = *attm.img_view.img;
      const ImageConfig& img_cfg = img.img_cfg;
      assert(attm.rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE);
      assert(img_cfg.width == width && img_cfg.height == height,
        "color attachment size mismatches framebuffer size");
      attm_img_views.emplace_back(img.img_view);
      break;
    }
    case L_ATTACHMENT_TYPE_DEPTH:
    {
      const DepthImage& depth_img = *attm.depth_img_view.depth_img;
      const DepthImageConfig& depth_img_cfg = depth_img.depth_img_cfg;
      assert(attm.rsc_view_ty == L_RESOURCE_VIEW_TYPE_DEPTH_IMAGE);
      assert(depth_img_cfg.width == width && depth_img_cfg.height == height,
        "depth attachment size mismatches framebuffer size");
      attm_img_views.emplace_back(depth_img.img_view);
      break;
    }
    default: panic("unexpected attachment type");
    }
  }

  VkFramebufferCreateInfo fci {};
  fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fci.renderPass = pass.pass;
  fci.attachmentCount = (uint32_t)attm_img_views.size();
  fci.pAttachments = attm_img_views.data();
  fci.width = width;
  fci.height = height;
  fci.layers = 1;

  VkFramebuffer framebuf;
  VK_ASSERT << vkCreateFramebuffer(pass.ctxt->dev, &fci, nullptr, &framebuf);

  return framebuf;
}
VkQueryPool _create_query_pool(
  const Context& ctxt,
  VkQueryType query_ty,
  uint32_t nquery
) {
  VkQueryPoolCreateInfo qpci {};
  qpci.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  qpci.queryType = query_ty;
  qpci.queryCount = nquery;

  VkQueryPool query_pool;
  VK_ASSERT << vkCreateQueryPool(ctxt.dev, &qpci, nullptr, &query_pool);

  return query_pool;
}
void _collect_task_invoke_transit(
  const std::vector<ResourceView> rsc_views,
  const std::vector<ResourceType> rsc_tys,
  InvocationTransitionDetail& transit_detail
) {
  assert(rsc_views.size() == rsc_tys.size());

  auto& buf_transit = transit_detail.buf_transit;
  auto& img_transit = transit_detail.img_transit;
  auto& depth_img_transit = transit_detail.depth_img_transit;

  for (size_t i = 0; i < rsc_views.size(); ++i) {
    const ResourceView& rsc_view = rsc_views[i];
    ResourceViewType rsc_view_ty = rsc_view.rsc_view_ty;
    ResourceType rsc_ty = rsc_tys[i];

    switch (rsc_ty) {
    case L_RESOURCE_TYPE_UNIFORM_BUFFER:
      if (rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER) {
        transit_detail.reg(rsc_view.buf_view, L_BUFFER_USAGE_UNIFORM_BIT);
      } else {
        unreachable();
      }
      break;
    case L_RESOURCE_TYPE_STORAGE_BUFFER:
      if (rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER) {
        transit_detail.reg(rsc_view.buf_view, L_BUFFER_USAGE_STORAGE_BIT);
      } else {
        unreachable();
      }
      break;
    case L_RESOURCE_TYPE_SAMPLED_IMAGE:
      if (rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE) {
        transit_detail.reg(rsc_view.img_view, L_IMAGE_USAGE_SAMPLED_BIT);
      } else if (rsc_view_ty == L_RESOURCE_VIEW_TYPE_DEPTH_IMAGE) {
        transit_detail.reg(rsc_view.depth_img_view,
          L_DEPTH_IMAGE_USAGE_SAMPLED_BIT);
      } else {
        unreachable();
      }
      break;
    case L_RESOURCE_TYPE_STORAGE_IMAGE:
      if (rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE) {
        transit_detail.reg(rsc_view.img_view, L_IMAGE_USAGE_STORAGE_BIT);
      } else {
        unreachable();
      }
      break;
    default: unreachable();
    }
  }
}
void _merge_subinvoke_transits(
  const std::vector<const Invocation*>& subinvokes,
  InvocationTransitionDetail& transit_detail
) {
  for (const auto& subinvoke : subinvokes) {
    assert(subinvoke != nullptr);
    for (const auto& pair : subinvoke->transit_detail.buf_transit) {
      transit_detail.buf_transit.emplace_back(pair);
    }
    for (const auto& pair : subinvoke->transit_detail.img_transit) {
      transit_detail.img_transit.emplace_back(pair);
    }
    for (const auto& pair : subinvoke->transit_detail.depth_img_transit) {
      transit_detail.depth_img_transit.emplace_back(pair);
    }
  }
}
void _merge_subinvoke_transits(
  const Invocation* subinvoke,
  InvocationTransitionDetail& transit_detail
) {
  std::vector<const Invocation*> subinvokes { subinvoke };
  _merge_subinvoke_transits(subinvokes, transit_detail);
}
SubmitType _infer_submit_ty(const std::vector<const Invocation*>& subinvokes) {
  for (size_t i = 0; i < subinvokes.size(); ++i) {
    SubmitType submit_ty = subinvokes[i]->submit_ty;
    if (submit_ty != L_SUBMIT_TYPE_ANY) { return submit_ty; }
  }
  return L_SUBMIT_TYPE_ANY;
}
VkBufferCopy _make_bc(const BufferView& src, const BufferView& dst) {
  VkBufferCopy bc {};
  bc.srcOffset = src.offset;
  bc.dstOffset = dst.offset;
  bc.size = dst.size;
  return bc;
}
VkImageCopy _make_ic(const ImageView& src, const ImageView& dst) {
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
  return ic;
}
VkBufferImageCopy _make_bic(const BufferView& buf, const ImageView& img) {
  VkBufferImageCopy bic {};
  bic.bufferOffset = buf.offset;
  bic.bufferRowLength = 0;
  bic.bufferImageHeight = (uint32_t)img.img->img_cfg.height;
  bic.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bic.imageSubresource.mipLevel = 0;
  bic.imageSubresource.baseArrayLayer = 0;
  bic.imageSubresource.layerCount = 1;
  bic.imageOffset.x = img.x_offset;
  bic.imageOffset.y = img.y_offset;
  bic.imageExtent.width = img.width;
  bic.imageExtent.height = img.height;
  bic.imageExtent.depth = 1;
  return bic;
}
void _fill_transfer_b2b_invoke(
  const BufferView& src,
  const BufferView& dst,
  Invocation& out
) {
  InvocationCopyBufferToBufferDetail b2b_detail {};
  b2b_detail.bc = _make_bc(src, dst);
  b2b_detail.src = src.buf->buf;
  b2b_detail.dst = dst.buf->buf;

  out.b2b_detail =
    std::make_unique<InvocationCopyBufferToBufferDetail>(std::move(b2b_detail));

  out.transit_detail.reg(src, L_BUFFER_USAGE_TRANSFER_SRC_BIT);
  out.transit_detail.reg(dst, L_BUFFER_USAGE_TRANSFER_DST_BIT);
}
void _fill_transfer_b2i_invoke(
  const BufferView& src,
  const ImageView& dst,
  Invocation& out
) {
  InvocationCopyBufferToImageDetail b2i_detail {};
  b2i_detail.bic = _make_bic(src, dst);
  b2i_detail.src = src.buf->buf;
  b2i_detail.dst = dst.img->img;

  out.b2i_detail =
    std::make_unique<InvocationCopyBufferToImageDetail>(std::move(b2i_detail));

  out.transit_detail.reg(src, L_BUFFER_USAGE_TRANSFER_SRC_BIT);
  out.transit_detail.reg(dst, L_IMAGE_USAGE_TRANSFER_DST_BIT);
}
void _fill_transfer_i2b_invoke(
  const ImageView& src,
  const BufferView& dst,
  Invocation& out
) {
  InvocationCopyImageToBufferDetail i2b_detail {};
  i2b_detail.bic = _make_bic(dst, src);
  i2b_detail.src = src.img->img;
  i2b_detail.dst = dst.buf->buf;

  out.i2b_detail =
    std::make_unique<InvocationCopyImageToBufferDetail>(std::move(i2b_detail));

  out.transit_detail.reg(src, L_IMAGE_USAGE_TRANSFER_SRC_BIT);
  out.transit_detail.reg(dst, L_BUFFER_USAGE_TRANSFER_DST_BIT);
}
void _fill_transfer_i2i_invoke(
  const ImageView& src,
  const ImageView& dst,
  Invocation& out
) {
  InvocationCopyImageToImageDetail i2i_detail {};
  i2i_detail.ic = _make_ic(src, dst);
  i2i_detail.src = src.img->img;
  i2i_detail.dst = dst.img->img;

  out.i2i_detail =
    std::make_unique<InvocationCopyImageToImageDetail>(std::move(i2i_detail));

  out.transit_detail.reg(src, L_IMAGE_USAGE_TRANSFER_SRC_BIT);
  out.transit_detail.reg(dst, L_IMAGE_USAGE_TRANSFER_DST_BIT);
}
Invocation create_trans_invoke(
  const Context& ctxt,
  const TransferInvocationConfig& cfg
) {
  const ResourceView& src_rsc_view = cfg.src_rsc_view;
  const ResourceView& dst_rsc_view = cfg.dst_rsc_view;
  ResourceViewType src_rsc_view_ty = src_rsc_view.rsc_view_ty;
  ResourceViewType dst_rsc_view_ty = dst_rsc_view.rsc_view_ty;

  Invocation out {};
  out.label = cfg.label;
  out.ctxt = &ctxt;
  out.submit_ty = L_SUBMIT_TYPE_TRANSFER;
  out.query_pool = cfg.is_timed ?
    _create_query_pool(ctxt, VK_QUERY_TYPE_TIMESTAMP, 2) : VK_NULL_HANDLE;

  if (
    src_rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER &&
    dst_rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER
  ) {
    _fill_transfer_b2b_invoke(
      src_rsc_view.buf_view, dst_rsc_view.buf_view, out);
  } else if (
    src_rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER &&
    dst_rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE
  ) {
    _fill_transfer_b2i_invoke(
      src_rsc_view.buf_view, dst_rsc_view.img_view, out);
  } else if (
    src_rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE &&
    dst_rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER
  ) {
    _fill_transfer_i2b_invoke(
      src_rsc_view.img_view, dst_rsc_view.buf_view, out);
  } else if (
    src_rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE &&
    dst_rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE
  ) {
    _fill_transfer_i2i_invoke(
      src_rsc_view.img_view, dst_rsc_view.img_view, out);
  } else {
    panic("depth image cannot be transferred");
  }

  log::debug("created transfer invocation");
  return out;
}
Invocation create_comp_invoke(
  const Task& task,
  const ComputeInvocationConfig& cfg
) {
  assert(task.rsc_tys.size() == cfg.rsc_views.size());
  assert(task.submit_ty == L_SUBMIT_TYPE_COMPUTE);
  const Context& ctxt = *task.ctxt;

  Invocation out {};
  out.label = cfg.label;
  out.ctxt = &ctxt;
  out.submit_ty = L_SUBMIT_TYPE_COMPUTE;
  out.query_pool = cfg.is_timed ?
    _create_query_pool(ctxt, VK_QUERY_TYPE_TIMESTAMP, 2) : VK_NULL_HANDLE;

  InvocationTransitionDetail transit_detail {};
  _collect_task_invoke_transit(cfg.rsc_views, task.rsc_tys, transit_detail);
  out.transit_detail = std::move(transit_detail);

  InvocationComputeDetail comp_detail {};
  comp_detail.task = &task;
  comp_detail.bind_pt = VK_PIPELINE_BIND_POINT_COMPUTE;
  if (task.desc_pool_sizes.size() > 0) {
    comp_detail.desc_pool = _create_desc_pool(ctxt, task.desc_pool_sizes);
    comp_detail.desc_set =
      _alloc_desc_set(ctxt, comp_detail.desc_pool, task.desc_set_layout);
    _update_desc_set(ctxt, comp_detail.desc_set, task.rsc_tys, cfg.rsc_views);
  }
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
  assert(task.rsc_tys.size() == cfg.rsc_views.size());
  assert(task.submit_ty == L_SUBMIT_TYPE_GRAPHICS);
  const Context& ctxt = *task.ctxt;

  Invocation out {};
  out.label = cfg.label;
  out.ctxt = &ctxt;
  out.submit_ty = L_SUBMIT_TYPE_GRAPHICS;
  out.query_pool = cfg.is_timed ?
    _create_query_pool(ctxt, VK_QUERY_TYPE_TIMESTAMP, 2) : VK_NULL_HANDLE;

  InvocationTransitionDetail transit_detail {};
  _collect_task_invoke_transit(cfg.rsc_views, task.rsc_tys, transit_detail);
  for (size_t i = 0; i < cfg.vert_bufs.size(); ++i) {
    transit_detail.reg(cfg.vert_bufs[i], L_BUFFER_USAGE_VERTEX_BIT);
  }
  if (cfg.nidx > 0) {
    transit_detail.reg(cfg.idx_buf, L_BUFFER_USAGE_INDEX_BIT);
  }
  out.transit_detail = std::move(transit_detail);

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
  graph_detail.task = &task;
  graph_detail.bind_pt = VK_PIPELINE_BIND_POINT_GRAPHICS;
  if (task.desc_pool_sizes.size() > 0) {
    graph_detail.desc_pool = _create_desc_pool(ctxt, task.desc_pool_sizes);
    graph_detail.desc_set =
      _alloc_desc_set(ctxt, graph_detail.desc_pool, task.desc_set_layout);
    _update_desc_set(ctxt, graph_detail.desc_set, task.rsc_tys, cfg.rsc_views);
  }
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
Invocation create_pass_invoke(
  const RenderPass& pass,
  const RenderPassInvocationConfig& cfg
) {
  const Context& ctxt = *pass.ctxt;

  Invocation out {};
  out.label = cfg.label;
  out.ctxt = &ctxt;
  out.submit_ty = L_SUBMIT_TYPE_GRAPHICS;
  out.query_pool = cfg.is_timed ?
    _create_query_pool(ctxt, VK_QUERY_TYPE_TIMESTAMP, 2) : VK_NULL_HANDLE;

  InvocationTransitionDetail transit_detail {};
  for (size_t i = 0; i < cfg.attms.size(); ++i) {
    const ResourceView& attm = cfg.attms[i];

    switch (attm.rsc_view_ty) {
    case L_RESOURCE_VIEW_TYPE_IMAGE:
      transit_detail.reg(attm.img_view, L_IMAGE_USAGE_ATTACHMENT_BIT);
      break;
    case L_RESOURCE_VIEW_TYPE_DEPTH_IMAGE:
      transit_detail.reg(attm.depth_img_view,
        L_DEPTH_IMAGE_USAGE_ATTACHMENT_BIT);
      break;
    default: panic("render pass attachment must be image or depth image");
    }
  }
  _merge_subinvoke_transits(cfg.invokes, transit_detail);
  out.transit_detail = std::move(transit_detail);

  InvocationRenderPassDetail pass_detail {};
  pass_detail.pass = &pass;
  pass_detail.framebuf = _create_framebuf(pass, cfg.attms);
  // TODO: (penguinliong) Command buffer baking.
  pass_detail.is_baked = false;
  pass_detail.subinvokes = cfg.invokes;

  for (size_t i = 0; i < cfg.invokes.size(); ++i) {
    const Invocation& invoke = *cfg.invokes[i];
    assert(invoke.graph_detail != nullptr,
      "render pass invocation constituent must be graphics task invocation");
  }

  out.pass_detail =
    std::make_unique<InvocationRenderPassDetail>(std::move(pass_detail));

  log::debug("created render pass invocation");
  return out;
}
Invocation create_composite_invoke(
  const Context& ctxt,
  const CompositeInvocationConfig& cfg
) {
  assert(cfg.invokes.size() > 0);

  Invocation out {};
  out.label = cfg.label;
  out.ctxt = &ctxt;
  out.submit_ty = _infer_submit_ty(cfg.invokes);
  out.query_pool = cfg.is_timed ?
    _create_query_pool(ctxt, VK_QUERY_TYPE_TIMESTAMP, 2) : VK_NULL_HANDLE;

  InvocationTransitionDetail transit_detail {};
  _merge_subinvoke_transits(cfg.invokes, transit_detail);
  out.transit_detail = std::move(transit_detail);

  InvocationCompositeDetail composite_detail {};
  composite_detail.subinvokes = cfg.invokes;

  out.composite_detail =
    std::make_unique<InvocationCompositeDetail>(std::move(composite_detail));

  log::debug("created composition invocation");
  return out;
}
void destroy_invoke(Invocation& invoke) {
  const Context& ctxt = *invoke.ctxt;
  if (
    invoke.b2b_detail ||
    invoke.b2i_detail ||
    invoke.i2b_detail ||
    invoke.i2i_detail
  ) {
    log::debug("destroyed transfer invocation '", invoke.label, "'");
  }
  if (invoke.comp_detail) {
    const InvocationComputeDetail& comp_detail = *invoke.comp_detail;
    vkDestroyDescriptorPool(ctxt.dev, comp_detail.desc_pool, nullptr);
    log::debug("destroyed compute invocation '", invoke.label, "'");
  }
  if (invoke.graph_detail) {
    const InvocationGraphicsDetail& graph_detail = *invoke.graph_detail;
    vkDestroyDescriptorPool(ctxt.dev, graph_detail.desc_pool, nullptr);
    log::debug("destroyed graphics invocation '", invoke.label, "'");
  }
  if (invoke.pass_detail) {
    const InvocationRenderPassDetail& pass_detail = *invoke.pass_detail;
    vkDestroyFramebuffer(ctxt.dev, pass_detail.framebuf, nullptr);
    log::debug("destroyed render pass invocation '", invoke.label, "'");
  }
  if (invoke.composite_detail) {
    log::debug("destroyed composite invocation '", invoke.label, "'");
  }

  if (invoke.query_pool != VK_NULL_HANDLE) {
    vkDestroyQueryPool(ctxt.dev, invoke.query_pool, nullptr);
    log::debug("destroyed timing objects");
  }

  if (invoke.bake_detail) {
    const InvocationBakingDetail& bake_detail = *invoke.bake_detail;
    vkDestroyCommandPool(ctxt.dev, bake_detail.cmd_pool, nullptr);
    log::debug("destroyed baking artifacts");
  }

  invoke = {};
}

double get_invoke_time_us(const Invocation& invoke) {
  if (invoke.query_pool == VK_NULL_HANDLE) { return 0.0; }
  uint64_t t[2];
  VK_ASSERT << vkGetQueryPoolResults(invoke.ctxt->dev, invoke.query_pool,
    0, 2, sizeof(uint64_t) * 2, &t, sizeof(uint64_t),
    VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT); // Wait till ready.
  double ns_per_tick = invoke.ctxt->physdev_prop.limits.timestampPeriod;
  return (t[1] - t[0]) * ns_per_tick / 1000.0;
}

VkSemaphore _create_sema(const Context& ctxt) {
  VkSemaphoreCreateInfo sci {};
  sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkSemaphore sema;
  VK_ASSERT << vkCreateSemaphore(ctxt.dev, &sci, nullptr, &sema);
  return sema;
}

VkCommandPool _create_cmd_pool(const Context& ctxt, SubmitType submit_ty) {
  VkCommandPoolCreateInfo cpci {};
  cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cpci.queueFamilyIndex = ctxt.submit_details.at(submit_ty).qfam_idx;

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
  if (level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
    submit_detail.signal_sema = VK_NULL_HANDLE;
  } else {
    submit_detail.signal_sema = _create_sema(ctxt);
  }

  submit_details.emplace_back(std::move(submit_detail));
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
  VkQueue queue = ctxt.submit_details.at(submit_detail.submit_ty).queue;
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
  const auto& submit_detail = transact.ctxt->submit_details.at(submit_ty);
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

void _make_buf_barrier_params(
  BufferUsage usage,
  VkAccessFlags& access,
  VkPipelineStageFlags& stage
) {
  switch (usage) {
  case L_BUFFER_USAGE_NONE:
    return; // Keep their original values.
  case L_BUFFER_USAGE_TRANSFER_SRC_BIT:
    access = VK_ACCESS_TRANSFER_READ_BIT;
    stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    break;
  case L_BUFFER_USAGE_TRANSFER_DST_BIT:
    access = VK_ACCESS_TRANSFER_WRITE_BIT;
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
  case L_IMAGE_USAGE_TRANSFER_SRC_BIT:
    access = VK_ACCESS_TRANSFER_READ_BIT;
    stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    break;
  case L_IMAGE_USAGE_TRANSFER_DST_BIT:
    access = VK_ACCESS_TRANSFER_WRITE_BIT;
    stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
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
const BufferView& _transit_rsc(
  TransactionLike& transact,
  const BufferView& buf_view,
  BufferUsage dst_usage
) {
  auto& dyn_detail = (BufferDynamicDetail&)buf_view.buf->dyn_detail;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  VkAccessFlags src_access = dyn_detail.access;
  VkPipelineStageFlags src_stage = dyn_detail.stage;

  VkAccessFlags dst_access = 0;
  VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  _make_buf_barrier_params(dst_usage, dst_access, dst_stage);

  if (src_access == dst_access && src_stage == dst_stage) {
    return buf_view;
  }

  VkBufferMemoryBarrier bmb {};
  bmb.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bmb.buffer = buf_view.buf->buf;
  bmb.srcAccessMask = src_access;
  bmb.dstAccessMask = dst_access;
  bmb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bmb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bmb.offset = buf_view.offset;
  bmb.size = buf_view.size;

  vkCmdPipelineBarrier(
    cmdbuf,
    src_stage,
    dst_stage,
    0,
    0, nullptr,
    1, &bmb,
    0, nullptr);

  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    log::debug("inserted buffer barrier");
  }

  dyn_detail.access = dst_access;
  dyn_detail.stage = dst_stage;

  return buf_view;
}
const ImageView& _transit_rsc(
  TransactionLike& transact,
  const ImageView& img_view,
  ImageUsage dst_usage
) {
  auto& dyn_detail = (ImageDynamicDetail&)img_view.img->dyn_detail;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  VkAccessFlags src_access = dyn_detail.access;
  VkPipelineStageFlags src_stage = dyn_detail.stage;
  VkImageLayout src_layout = dyn_detail.layout;

  VkAccessFlags dst_access = 0;
  VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkImageLayout dst_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  _make_img_barrier_params(dst_usage, dst_access, dst_stage, dst_layout);

  if (
    src_access == dst_access &&
    src_stage == dst_stage &&
    src_layout == dst_layout
  ) {
    return img_view;
  }

  VkImageMemoryBarrier imb {};
  imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imb.image = img_view.img->img;
  imb.srcAccessMask = src_access;
  imb.dstAccessMask = dst_access;
  imb.oldLayout = src_layout;
  imb.newLayout = dst_layout;
  imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imb.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  // TODO: (penguinliong) Multi-layer image. 
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
    log::debug("inserted image barrier");
  }

  dyn_detail.access = dst_access;
  dyn_detail.stage = dst_stage;
  dyn_detail.layout = dst_layout;

  return img_view;
}
const DepthImageView& _transit_rsc(
  TransactionLike& transact,
  const DepthImageView& depth_img_view,
  DepthImageUsage dst_usage
) {
  auto& dyn_detail =
    (DepthImageDynamicDetail&)depth_img_view.depth_img->dyn_detail;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  VkAccessFlags src_access = dyn_detail.access;
  VkPipelineStageFlags src_stage = dyn_detail.stage;
  VkImageLayout src_layout = dyn_detail.layout;

  VkAccessFlags dst_access = 0;
  VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkImageLayout dst_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  _make_depth_img_barrier_params(dst_usage, dst_access, dst_stage, dst_layout);

  if (
    src_access == dst_access &&
    src_stage == dst_stage &&
    src_layout == dst_layout
  ) {
    return depth_img_view;
  }

  VkImageMemoryBarrier imb {};
  imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imb.image = depth_img_view.depth_img->img;
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
    log::debug("inserted depth image barrier");
  }

  dyn_detail.access = dst_access;
  dyn_detail.stage = dst_stage;
  dyn_detail.layout = dst_layout;

  return depth_img_view;
}
void _transit_rscs(
  TransactionLike& transact,
  const InvocationTransitionDetail& transit_detail
) {
  // Transition all referenced resources.
  for (auto& pair : transit_detail.buf_transit) {
    _transit_rsc(transact, pair.first, pair.second);
  }
  for (auto& pair : transit_detail.img_transit) {
    _transit_rsc(transact, pair.first, pair.second);
  }
  for (auto& pair : transit_detail.depth_img_transit) {
    _transit_rsc(transact, pair.first, pair.second);
  }
}

void _record_invoke(TransactionLike& transact, const Invocation& invoke) {
  VkCommandBuffer cmdbuf = _get_cmdbuf(transact, invoke.submit_ty);

  // If the invocation has been baked, simply inline the baked secondary command
  // buffer.
  if (invoke.bake_detail) {
    vkCmdExecuteCommands(cmdbuf, 1, &invoke.bake_detail->cmdbuf);
    return;
  }

  if (invoke.query_pool != VK_NULL_HANDLE) {
    vkCmdResetQueryPool(cmdbuf, invoke.query_pool, 0, 2);
    vkCmdWriteTimestamp(cmdbuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      invoke.query_pool, 0);

    log::debug("invocation '", invoke.label, "' will be timed");
  }

  _transit_rscs(transact, invoke.transit_detail);

  if (invoke.b2b_detail) {
    const InvocationCopyBufferToBufferDetail& b2b_detail =
      *invoke.b2b_detail;
    vkCmdCopyBuffer(cmdbuf, b2b_detail.src, b2b_detail.dst, 1, &b2b_detail.bc);
    log::debug("applied transfer invocation '", invoke.label, "'");

  } else if (invoke.b2i_detail) {
    const InvocationCopyBufferToImageDetail& b2i_detail = *invoke.b2i_detail;
    vkCmdCopyBufferToImage(cmdbuf, b2i_detail.src, b2i_detail.dst,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &b2i_detail.bic);
    log::debug("applied transfer invocation '", invoke.label, "'");

  } else if (invoke.i2b_detail) {
    const InvocationCopyImageToBufferDetail& i2b_detail = *invoke.i2b_detail;
    vkCmdCopyImageToBuffer(cmdbuf, i2b_detail.src,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, i2b_detail.dst, 1, &i2b_detail.bic);
    log::debug("applied transfer invocation '", invoke.label, "'");

  } else if (invoke.i2i_detail) {
    const InvocationCopyImageToImageDetail& i2i_detail = *invoke.i2i_detail;
    vkCmdCopyImage(cmdbuf, i2i_detail.src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      i2i_detail.dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &i2i_detail.ic);
    log::debug("applied transfer invocation '", invoke.label, "'");

  } else if (invoke.comp_detail) {
    const InvocationComputeDetail& comp_detail = *invoke.comp_detail;
    const Task& task = *comp_detail.task;
    const DispatchSize& workgrp_count = comp_detail.workgrp_count;

    vkCmdBindPipeline(cmdbuf, comp_detail.bind_pt, task.pipe);
    if (comp_detail.desc_set != VK_NULL_HANDLE) {
      vkCmdBindDescriptorSets(cmdbuf, comp_detail.bind_pt, task.pipe_layout,
        0, 1, &comp_detail.desc_set, 0, nullptr);
    }
    vkCmdDispatch(cmdbuf, workgrp_count.x, workgrp_count.y, workgrp_count.z);
    log::debug("applied compute invocation '", invoke.label, "'");

  } else if (invoke.graph_detail) {
    const InvocationGraphicsDetail& graph_detail = *invoke.graph_detail;
    const Task& task = *graph_detail.task;

    vkCmdBindPipeline(cmdbuf, graph_detail.bind_pt, task.pipe);
    if (graph_detail.desc_set != VK_NULL_HANDLE) {
      vkCmdBindDescriptorSets(cmdbuf, graph_detail.bind_pt, task.pipe_layout,
        0, 1, &graph_detail.desc_set, 0, nullptr);
    }
    // TODO: (penguinliong) Vertex, index buffer transition.
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
    log::debug("applied graphics invocation '", invoke.label, "'");

  } else if (invoke.pass_detail) {
    const InvocationRenderPassDetail& pass_detail = *invoke.pass_detail;
    const RenderPass& pass = *pass_detail.pass;
    const std::vector<const Invocation*>& subinvokes = pass_detail.subinvokes;

    VkSubpassContents sc = subinvokes.size() > 0 && subinvokes[0]->bake_detail ?
      VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS :
      VK_SUBPASS_CONTENTS_INLINE;

    VkRenderPassBeginInfo rpbi {};
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass = pass.pass;
    rpbi.framebuffer = pass_detail.framebuf;
    rpbi.renderArea.extent = pass.viewport.extent;
    rpbi.clearValueCount = (uint32_t)pass.clear_values.size();
    rpbi.pClearValues = pass.clear_values.data();
    vkCmdBeginRenderPass(cmdbuf, &rpbi, sc);
    log::debug("render pass invocation '", invoke.label, "' began");

    for (size_t i = 0; i < pass_detail.subinvokes.size(); ++i) {
      if (i > 0) {
        sc = subinvokes[i]->bake_detail ?
          VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS :
          VK_SUBPASS_CONTENTS_INLINE;
        vkCmdNextSubpass(cmdbuf, sc);
        log::debug("render pass invocation '", invoke.label, "' switched to a "
          "next subpass");
      }

      const Invocation* subinvoke = pass_detail.subinvokes[i];
      assert(subinvoke != nullptr, "null subinvocation is not allowed");
      _record_invoke(transact, *subinvoke);
    }
    vkCmdEndRenderPass(cmdbuf);
    log::debug("render pass invocation '", invoke.label, "' ended");

  } else if (invoke.composite_detail) {
    const InvocationCompositeDetail& composite_detail =
      *invoke.composite_detail;

    log::debug("composite invocation '", invoke.label, "' began");

    for (size_t i = 0; i < composite_detail.subinvokes.size(); ++i) {
      const Invocation* subinvoke = composite_detail.subinvokes[i];
      assert(subinvoke != nullptr, "null subinvocation is not allowed");
      _record_invoke(transact, *subinvoke);
    }

    log::debug("composite invocation '", invoke.label, "' ended");

  } else {
    unreachable();
  }

  if (invoke.query_pool != VK_NULL_HANDLE) {
    vkCmdWriteTimestamp(cmdbuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      invoke.query_pool, 1);
  }

  log::debug("scheduled invocation '", invoke.label, "' for execution");
}

bool _can_bake_invoke(const Invocation& invoke) {
  // Render pass is never baked, enforced by Vulkan specification.
  if (invoke.pass_detail != nullptr) { return false; }

  if (invoke.composite_detail != nullptr) {
    uint32_t submit_ty = ~uint32_t(0);
    for (const Invocation* subinvoke : invoke.composite_detail->subinvokes) {
      // If a subinvocation cannot be baked, this invocation too cannot.
      if (!_can_bake_invoke(*subinvoke)) { return false; }
      submit_ty &= subinvoke->submit_ty;
    }
    // Multiple subinvocations but their submit types mismatch.
    if (submit_ty == 0) { return 0; }
  }

  return true;
}
void bake_invoke(Invocation& invoke) {
  if (!_can_bake_invoke(invoke)) { return; }

  TransactionLike transact {};
  transact.ctxt = invoke.ctxt;
  transact.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  _record_invoke(transact, invoke);
  _end_cmdbuf(transact.submit_details.back());

  assert(transact.submit_details.size() == 1);
  const TransactionSubmitDetail& submit_detail = transact.submit_details[0];
  assert(submit_detail.submit_ty == invoke.submit_ty);
  assert(submit_detail.signal_sema == VK_NULL_HANDLE);

  InvocationBakingDetail bake_detail {};
  bake_detail.cmd_pool = submit_detail.cmd_pool;
  bake_detail.cmdbuf = submit_detail.cmdbuf;

  invoke.bake_detail =
    std::make_unique<InvocationBakingDetail>(std::move(bake_detail));

  log::debug("baked invocation '", invoke.label, "'");
}



VkFence _create_fence(const Context& ctxt) {
  VkFenceCreateInfo fci {};
  fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  VkFence fence;
  VK_ASSERT << vkCreateFence(ctxt.dev, &fci, nullptr, &fence);
  return fence;
}
Transaction create_transact(const Invocation& invoke) {
  const Context& ctxt = *invoke.ctxt;

  TransactionLike transact {};
  transact.ctxt = invoke.ctxt;
  transact.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  util::Timer timer {};
  timer.tic();
  _record_invoke(transact, invoke);
  _end_cmdbuf(transact.submit_details.back());
  timer.toc();

  Transaction out {};
  out.invoke = &invoke;
  out.submit_details = std::move(transact.submit_details);
  out.fence = _create_fence(ctxt);

  _submit_transact_submit_detail(ctxt, out.submit_details.back(), out.fence);

  log::debug("created and submitted transaction for execution, command "
    "recording took ", timer.us(), "us");
  return out;
}
void destroy_transact(Transaction& transact) {
  const Context& ctxt = *transact.invoke->ctxt;
  if (transact.fence != VK_NULL_HANDLE) {
    for (auto& submit_detail : transact.submit_details) {
      if (submit_detail.signal_sema != VK_NULL_HANDLE) {
        vkDestroySemaphore(ctxt.dev, submit_detail.signal_sema, nullptr);
      }
      if (submit_detail.cmd_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(ctxt.dev, submit_detail.cmd_pool, nullptr);
      }
    }
    vkDestroyFence(ctxt.dev, transact.fence, nullptr);
    log::debug("destroyed transaction");
  }
  transact = {};
}
bool is_transact_done(const Transaction& transact) {
  const Context& ctxt = *transact.invoke->ctxt;
  VkResult err = vkGetFenceStatus(ctxt.dev, transact.fence);
  if (err == VK_NOT_READY) {
    return false;
  } else {
    VK_ASSERT << err;
    return true;
  }
}
void wait_transact(const Transaction& transact) {
  const uint32_t SPIN_INTERVAL = 3000;

  const Context& ctxt = *transact.invoke->ctxt;

  util::Timer wait_timer {};
  wait_timer.tic();
  for (VkResult err;;) {
    err = vkWaitForFences(ctxt.dev, 1, &transact.fence, VK_TRUE,
        SPIN_INTERVAL);
    if (err == VK_TIMEOUT) {
      // log::warn("timeout after 3000ns");
    } else {
      VK_ASSERT << err;
      break;
    }
  }
  wait_timer.toc();

  log::debug("command drain returned after ", wait_timer.us(), "us since the "
    "wait started (spin interval = ", SPIN_INTERVAL / 1000.0, "us");
}

} // namespace HAL_IMPL_NAMESPACE

} // namespace liong

#endif // GFT_WITH_VULKAN
