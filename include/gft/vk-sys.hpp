#pragma once
#include <memory>
#include <vector>
#include "vulkan/vulkan.h"
#include "vk_mem_alloc.h"

namespace liong {
namespace vk {

class VkException : public std::exception {
  std::string msg;
public:
  inline VkException(VkResult code) {
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

  inline const char* what() const noexcept override {
    return msg.c_str(); 
  }
};
struct VkAssert {
  inline const VkAssert& operator<<(VkResult code) const {
    if (code != VK_SUCCESS) { throw VkException(code); }
    return *this;
  }
};
#define VK_ASSERT (::liong::vk::VkAssert{})


namespace sys {

struct Fence {
  typedef Fence Self;
  VkDevice dev;
  VkFence fence;

  Fence() : dev(VK_NULL_HANDLE), fence(VK_NULL_HANDLE) {}
  ~Fence() { destroy(); }

  static std::shared_ptr<Fence> create(VkDevice dev) {
    VkFenceCreateInfo fci {};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    return create(dev, &fci);
  }
  static std::shared_ptr<Fence> create(VkDevice dev, const VkFenceCreateInfo* fci) {
    VkFence fence = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateFence(dev, fci, nullptr, &fence);

    Fence out {};
    out.dev = dev;
    out.fence = fence;
    return std::make_shared<Fence>(std::move(out));
  }
  void destroy() {
    vkDestroyFence(dev, fence, nullptr);
  }
};
typedef std::shared_ptr<Fence> FenceRef;

struct Semaphore {
  typedef Semaphore Self;
  VkDevice dev;
  VkSemaphore sema;

  Semaphore() : dev(VK_NULL_HANDLE), sema(VK_NULL_HANDLE) {}
  ~Semaphore() { destroy(); }

  static std::shared_ptr<Semaphore> create(VkDevice dev) {
    VkSemaphoreCreateInfo sci {};
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    return Self::create(dev, &sci);
  }
  static std::shared_ptr<Semaphore> create(VkDevice dev, const VkSemaphoreCreateInfo* sci) {
    VkSemaphore sema = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateSemaphore(dev, sci, nullptr, &sema);

    Semaphore out {};
    out.dev = dev;
    out.sema = sema;
    return std::make_shared<Semaphore>(std::move(out));
  }
  void destroy() {
    vkDestroySemaphore(dev, sema, nullptr);
  }
};
typedef std::shared_ptr<Semaphore> SemaphoreRef;

} // namespace sys
} // namespace vk
} // namespace liong::vk::sys
