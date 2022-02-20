#include "gft/vk.hpp"
#include "gft/log.hpp"

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

  VmaAllocationCreateInfo aci {};
  aci.usage = vma_usage;
  VK_ASSERT <<
    vmaCreateBuffer(ctxt.allocator, &bci, &aci, &buf, &alloc, nullptr);

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


} // namespace vk
} // namespace liong
