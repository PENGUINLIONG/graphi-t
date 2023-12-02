#pragma once
#include "gft/hal/buffer.hpp"
#include "gft/vk/vk-context.hpp"

namespace liong {
namespace vk {

struct VulkanBuffer;
typedef std::shared_ptr<VulkanBuffer> VulkanBufferRef;

struct BufferDynamicDetail {
  VkPipelineStageFlags stage;
  VkAccessFlags access;
};
struct VulkanBuffer : public Buffer {
  VulkanContextRef ctxt;

  sys::BufferRef buf;
  BufferDynamicDetail dyn_detail;

  static BufferRef create(const ContextRef& ctxt, const BufferConfig& cfg);
  VulkanBuffer(VulkanContextRef ctxt, BufferInfo&& info);
  ~VulkanBuffer();

  void* map(MemoryAccess map_access);
  void unmap();

  inline static VulkanBufferRef from_hal(const BufferRef& ref) {
    return std::static_pointer_cast<VulkanBuffer>(ref);
  }
};

} // namespace vk
} // namespace liong
