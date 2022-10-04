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

struct Instance {
  typedef Instance Self;
  VkInstance inst;
  bool should_destroy;

  Instance(VkInstance inst, bool should_destroy) :
    inst(inst), should_destroy(should_destroy) {}
  ~Instance() { destroy(); }

  static std::shared_ptr<Instance> create(const VkInstanceCreateInfo* ici) {
    VkInstance inst = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateInstance(ici, nullptr, &inst);
    return std::make_shared<Instance>(inst, true);
  }
  void destroy() {
    vkDestroyInstance(inst, nullptr);
  }
};
typedef std::shared_ptr<Instance> InstanceRef;

struct Fence {
  typedef Fence Self;
  VkDevice dev;
  VkFence fence;
  bool should_destroy;

  Fence(VkDevice dev, VkFence fence, bool should_destroy) :
    dev(dev), fence(fence), should_destroy(should_destroy) {}
  ~Fence() { destroy(); }

  static std::shared_ptr<Fence> create(VkDevice dev) {
    VkFenceCreateInfo fci {};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    return create(dev, &fci);
  }
  static std::shared_ptr<Fence> create(VkDevice dev, const VkFenceCreateInfo* fci) {
    VkFence fence = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateFence(dev, fci, nullptr, &fence);

    return std::make_shared<Fence>(dev, fence, true);
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
  bool should_destroy;

  Semaphore(VkDevice dev, VkSemaphore sema, bool should_destroy) :
    dev(dev), sema(sema), should_destroy(should_destroy) {}
  ~Semaphore() { destroy(); }

  static std::shared_ptr<Semaphore> create(VkDevice dev) {
    VkSemaphoreCreateInfo sci {};
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    return Self::create(dev, &sci);
  }
  static std::shared_ptr<Semaphore> create(VkDevice dev, const VkSemaphoreCreateInfo* sci) {
    VkSemaphore sema = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateSemaphore(dev, sci, nullptr, &sema);

    return std::make_shared<Semaphore>(dev, sema, true);
  }
  void destroy() {
    vkDestroySemaphore(dev, sema, nullptr);
  }
};
typedef std::shared_ptr<Semaphore> SemaphoreRef;

} // namespace sys
} // namespace vk
} // namespace liong::vk::sys
