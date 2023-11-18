#include "sys.hpp"
#include "gft/assert.hpp"
#include "gft/util.hpp"
#include "gft/log.hpp"
#include "gft/vk/vk-buffer.hpp"

namespace liong {
namespace vk {

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
BufferRef VulkanBuffer::create(const ContextRef &ctxt,
                               const BufferConfig &cfg) {
  VulkanContextRef ctxt_ = VulkanContext::from_hal(ctxt);

  VkBufferCreateInfo bci {};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (cfg.usage & L_BUFFER_USAGE_TRANSFER_SRC_BIT) {
    bci.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  }
  if (cfg.usage & L_BUFFER_USAGE_TRANSFER_DST_BIT) {
    bci.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  if (cfg.usage & L_BUFFER_USAGE_UNIFORM_BIT) {
    bci.usage |=
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  if (cfg.usage & L_BUFFER_USAGE_STORAGE_BIT) {
    bci.usage |=
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  if (cfg.usage & L_BUFFER_USAGE_VERTEX_BIT) {
    bci.usage |=
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  if (cfg.usage & L_BUFFER_USAGE_INDEX_BIT) {
    bci.usage |=
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  bci.size = cfg.size;

  VmaAllocationCreateInfo aci {};
  aci.usage = _host_access2vma_usage(cfg.host_access);

  sys::BufferRef buf = sys::Buffer::create(*ctxt_->allocator, &bci, &aci);

  BufferDynamicDetail dyn_detail {};
  dyn_detail.access = 0;
  dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;

  BufferInfo info{};
  info.label = cfg.label;
  info.host_access = cfg.host_access;
  info.size = cfg.size;
  info.usage = cfg.usage;

  VulkanBufferRef out = std::make_shared<VulkanBuffer>(ctxt_, std::move(info));
  out->ctxt = ctxt_;
  out->buf = std::move(buf);
  out->dyn_detail = std::move(dyn_detail);
  L_DEBUG("created buffer '", cfg.label, "'");

  return out;
}
VulkanBuffer::VulkanBuffer(VulkanContextRef ctxt, BufferInfo &&info)
  : Buffer(std::move(info)), ctxt(ctxt) {
}
VulkanBuffer::~VulkanBuffer() {
  if (buf) {
    L_DEBUG("destroyed buffer '", info.label, "'");
  }
}

void* VulkanBuffer::map(MemoryAccess map_access) {
  L_ASSERT(map_access != 0, "memory map access must be read, write or both");

  void* mapped = nullptr;
  VK_ASSERT << vmaMapMemory(*ctxt->allocator, buf->alloc, &mapped);

  dyn_detail.access = map_access == L_MEMORY_ACCESS_READ_BIT ?
    VK_ACCESS_HOST_READ_BIT : VK_ACCESS_HOST_WRITE_BIT;
  dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;

  L_DEBUG("mapped buffer '", info.label, "'");
  return (uint8_t*)mapped;
}
void VulkanBuffer::unmap() {
  vmaUnmapMemory(*ctxt->allocator, buf->alloc);
  L_DEBUG("unmapped buffer '", info.label, "'");
}

} // namespace vk
} // namespace liong
