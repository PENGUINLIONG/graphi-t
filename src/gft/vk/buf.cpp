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
    bci.usage |=
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  if (buf_cfg.usage & L_BUFFER_USAGE_VERTEX_BIT) {
    bci.usage |=
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  if (buf_cfg.usage & L_BUFFER_USAGE_INDEX_BIT) {
    bci.usage |=
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  bci.size = buf_cfg.size;

  VmaAllocationCreateInfo aci {};
  aci.usage = _host_access2vma_usage(buf_cfg.host_access);

  sys::BufferRef buf = sys::Buffer::create(ctxt.allocator, &bci, &aci);

  BufferDynamicDetail dyn_detail {};
  dyn_detail.access = 0;
  dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;

  L_DEBUG("created buffer '", buf_cfg.label, "'");
  return Buffer { &ctxt, std::move(buf), buf_cfg, std::move(dyn_detail) };
}
void destroy_buf(Buffer& buf) {
  if (buf.buf != VK_NULL_HANDLE) {
    buf.buf.reset();
    L_DEBUG("destroyed buffer '", buf.buf_cfg.label, "'");
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
  L_ASSERT(map_access != 0, "memory map access must be read, write or both");

  VK_ASSERT << vmaMapMemory(buf.buf->ctxt->allocator, buf.buf->buf->alloc, &mapped);

  auto& dyn_detail = (BufferDynamicDetail&)buf.buf->dyn_detail;
  dyn_detail.access = map_access == L_MEMORY_ACCESS_READ_BIT ?
    VK_ACCESS_HOST_READ_BIT : VK_ACCESS_HOST_WRITE_BIT;
  dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;

  L_DEBUG("mapped buffer '", buf.buf->buf_cfg.label, "' from ", buf.offset,
    " to ", buf.offset + buf.size);
  mapped = (uint8_t*)mapped + buf.offset;
}
void unmap_buf_mem(
  const BufferView& buf,
  void* mapped
) {
  vmaUnmapMemory(buf.buf->ctxt->allocator, buf.buf->buf->alloc);
  L_DEBUG("unmapped buffer '", buf.buf->buf_cfg.label, "'");
}
void read_buf_mem(
  const BufferView& buf,
  void* data,
  size_t size
) {
  L_ASSERT(size <= buf.size);
  L_ASSERT(buf.offset + buf.size <= buf.buf->buf_cfg.size);

  if (buf.buf->buf_cfg.host_access & L_MEMORY_ACCESS_READ_BIT) {
    void* mapped = nullptr;
    map_buf_mem(buf, L_MEMORY_ACCESS_READ_BIT, mapped);
    L_ASSERT(mapped != nullptr);
    std::memcpy(data, mapped, size);
    unmap_buf_mem(buf, mapped);
  } else {
    const Context& ctxt = *buf.buf->ctxt;

    BufferConfig staging_buf_cfg {};
    staging_buf_cfg.align = 1;
    staging_buf_cfg.host_access = L_MEMORY_ACCESS_READ_BIT;
    staging_buf_cfg.size = size;
    staging_buf_cfg.usage = L_BUFFER_USAGE_TRANSFER_DST_BIT;
    Buffer stage_buf = create_buf(ctxt, staging_buf_cfg);
    BufferView stage_buf_view = make_buf_view(stage_buf);

    TransferInvocationConfig trans_invoke_cfg {};
    trans_invoke_cfg.src_rsc_view = make_rsc_view(buf);
    trans_invoke_cfg.dst_rsc_view = make_rsc_view(stage_buf_view);
    Invocation invoke = create_trans_invoke(ctxt, trans_invoke_cfg);
    Transaction transact = submit_invoke(invoke);
    wait_transact(transact);

    read_buf_mem(stage_buf_view, data, size);

    destroy_buf(stage_buf);
  }
}


} // namespace vk
} // namespace liong
