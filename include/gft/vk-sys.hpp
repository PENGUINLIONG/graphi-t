#pragma once
#include <memory>
#include <string>
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
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      msg = "out of host memory";
      break;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      msg = "out of device memory";
      break;
    case VK_ERROR_INITIALIZATION_FAILED:
      msg = "initialization failed";
      break;
    case VK_ERROR_DEVICE_LOST:
      msg = "device lost";
      break;
    case VK_ERROR_MEMORY_MAP_FAILED:
      msg = "memory map failed";
      break;
    case VK_ERROR_LAYER_NOT_PRESENT:
      msg = "layer not supported";
      break;
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      msg = "extension may present";
      break;
    case VK_ERROR_INCOMPATIBLE_DRIVER:
      msg = "incompatible driver";
      break;
    case VK_ERROR_TOO_MANY_OBJECTS:
      msg = "too many objects";
      break;
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
      msg = "format not supported";
      break;
    case VK_ERROR_FRAGMENTED_POOL:
      msg = "fragmented pool";
      break;
    case VK_ERROR_OUT_OF_POOL_MEMORY:
      msg = "out of pool memory";
      break;
    default:
      msg = std::string("unknown vulkan error: ") + std::to_string(code);
      break;
    }
  }

  inline const char* what() const noexcept override {
    return msg.c_str();
  }
};
struct VkAssert {
  inline const VkAssert& operator<<(VkResult code) const {
    if (code != VK_SUCCESS) {
      throw VkException(code);
    }
    return *this;
  }
};
#define VK_ASSERT (::liong::vk::VkAssert {})

namespace sys {

struct Instance {
  typedef Instance Self;
  VkInstance inst;
  bool should_destroy;

  Instance(VkInstance inst, bool should_destroy) :
    inst(inst), should_destroy(should_destroy) {}
  ~Instance() {
    destroy();
  }

  operator VkInstance() const {
    return inst;
  }

  static std::shared_ptr<Instance> create(const VkInstanceCreateInfo* ici) {
    VkInstance inst = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateInstance(ici, nullptr, &inst);
    return std::make_shared<Instance>(inst, true);
  }
  void destroy() {
    if (should_destroy) {
      vkDestroyInstance(inst, nullptr);
    }
  }
};
typedef std::shared_ptr<Instance> InstanceRef;

struct Device {
  typedef Device Self;
  VkPhysicalDevice physdev;
  VkDevice dev;
  bool should_destroy;

  Device(VkPhysicalDevice physdev, VkDevice dev, bool should_destroy) :
    physdev(physdev), dev(dev), should_destroy(should_destroy) {}
  ~Device() {
    destroy();
  }

  operator VkDevice() const {
    return dev;
  }

  static std::shared_ptr<Device> create(
    VkPhysicalDevice physdev,
    const VkDeviceCreateInfo* dci
  ) {
    VkDevice dev = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateDevice(physdev, dci, nullptr, &dev);
    return std::make_shared<Device>(physdev, dev, true);
  }
  void destroy() {
    if (should_destroy) {
      vkDestroyDevice(dev, nullptr);
    }
  }
};
typedef std::shared_ptr<Device> DeviceRef;

struct Allocator {
  typedef Allocator Self;
  VmaAllocator allocator;
  bool should_destroy;

  Allocator(VmaAllocator allocator, bool should_destroy) :
    allocator(allocator), should_destroy(should_destroy) {}
  ~Allocator() {
    destroy();
  }

  operator VmaAllocator() const {
    return allocator;
  }

  static std::shared_ptr<Allocator> create(const VmaAllocatorCreateInfo* aci) {
    VmaAllocator allocator = VK_NULL_HANDLE;
    VK_ASSERT << vmaCreateAllocator(aci, &allocator);
    return std::make_shared<Allocator>(allocator, true);
  }
  void destroy() {
    if (should_destroy) {
      vmaDestroyAllocator(allocator);
    }
  }
};
typedef std::shared_ptr<Allocator> AllocatorRef;

struct Buffer {
  typedef Device Self;
  VmaAllocator allocator;
  VkBuffer buf;
  VmaAllocation alloc;
  bool should_destroy;

  Buffer(
    VmaAllocator allocator,
    VkBuffer buf,
    VmaAllocation alloc,
    bool should_destroy
  ) :
    allocator(allocator),
    buf(buf),
    alloc(alloc),
    should_destroy(should_destroy) {}
  ~Buffer() {
    destroy();
  }

  operator VkBuffer() const {
    return buf;
  }

  static std::shared_ptr<Buffer> create(
    VmaAllocator allocator,
    const VkBufferCreateInfo* bci,
    const VmaAllocationCreateInfo* aci
  ) {
    VkBuffer buf = VK_NULL_HANDLE;
    VmaAllocation alloc = VK_NULL_HANDLE;
    VK_ASSERT << vmaCreateBuffer(allocator, bci, aci, &buf, &alloc, nullptr);
    return std::make_shared<Buffer>(allocator, buf, alloc, true);
  }
  void destroy() {
    if (should_destroy) {
      vmaDestroyBuffer(allocator, buf, alloc);
    }
  }
};
typedef std::shared_ptr<Buffer> BufferRef;

struct Image {
  typedef Device Self;
  VmaAllocator allocator;
  VkImage img;
  VmaAllocation alloc;
  bool should_destroy;

  Image(
    VmaAllocator allocator,
    VkImage img,
    VmaAllocation alloc,
    bool should_destroy
  ) :
    allocator(allocator),
    img(img),
    alloc(alloc),
    should_destroy(should_destroy) {}
  ~Image() {
    destroy();
  }

  operator VkImage() const {
    return img;
  }

  static std::shared_ptr<Image> create(
    VmaAllocator allocator,
    const VkImageCreateInfo* ici,
    const VmaAllocationCreateInfo* aci
  ) {
    VkImage img = VK_NULL_HANDLE;
    VmaAllocation alloc = VK_NULL_HANDLE;
    VK_ASSERT << vmaCreateImage(allocator, ici, aci, &img, &alloc, nullptr);
    return std::make_shared<Image>(allocator, img, alloc, true);
  }
  void destroy() {
    if (should_destroy) {
      vmaDestroyImage(allocator, img, alloc);
    }
  }
};
typedef std::shared_ptr<Image> ImageRef;

struct ImageView {
  typedef ImageView Self;
  VkDevice dev;
  VkImageView img_view;
  bool should_destroy;

  ImageView(VkDevice dev, VkImageView img_view, bool should_destroy) :
    dev(dev), img_view(img_view), should_destroy(should_destroy) {}
  ~ImageView() {
    destroy();
  }

  operator VkImageView() const {
    return img_view;
  }

  static std::shared_ptr<ImageView> create(
    VkDevice dev,
    const VkImageViewCreateInfo* ivci
  ) {
    VkImageView img_view = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateImageView(dev, ivci, nullptr, &img_view);
    return std::make_shared<ImageView>(dev, img_view, true);
  }
  void destroy() {
    if (should_destroy) {
      vkDestroyImageView(dev, img_view, nullptr);
    }
  }
};
typedef std::shared_ptr<ImageView> ImageViewRef;

struct Sampler {
  typedef Sampler Self;
  VkDevice dev;
  VkSampler sampler;
  bool should_destroy;

  Sampler(VkDevice dev, VkSampler sampler, bool should_destroy) :
    dev(dev), sampler(sampler), should_destroy(should_destroy) {}
  ~Sampler() {
    destroy();
  }

  operator VkSampler() const {
    return sampler;
  }

  static std::shared_ptr<Sampler> create(
    VkDevice dev,
    const VkSamplerCreateInfo* sci
  ) {
    VkSampler sampler = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateSampler(dev, sci, nullptr, &sampler);
    return std::make_shared<Sampler>(dev, sampler, true);
  }
  void destroy() {
    if (should_destroy) {
      vkDestroySampler(dev, sampler, nullptr);
    }
  }
};
typedef std::shared_ptr<Sampler> SamplerRef;

struct Surface {
  typedef Surface Self;
  VkInstance inst;
  VkSurfaceKHR surf;
  bool should_destroy;

  Surface(VkInstance inst, VkSurfaceKHR surf, bool should_destroy) :
    inst(inst), surf(surf), should_destroy(should_destroy) {}
  ~Surface() {
    destroy();
  }

  operator VkSurfaceKHR() const {
    return surf;
  }

#if VK_KHR_win32_surface
  static std::shared_ptr<Surface> create(
    VkInstance inst,
    const VkWin32SurfaceCreateInfoKHR* wsci
  ) {
    VkSurfaceKHR surf = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateWin32SurfaceKHR(inst, wsci, nullptr, &surf);
    return std::make_shared<Surface>(inst, surf, true);
  }
#endif // VK_KHR_win32_surface
#if VK_KHR_android_surface
  static std::shared_ptr<Surface> create(
    VkInstance inst,
    const VkAndroidSurfaceCreateInfoKHR* asci
  ) {
    VkSurfaceKHR surf = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateAndroidSurfaceKHR(inst, asci, nullptr, &surf);
    return std::make_shared<Surface>(inst, surf, true);
  }
#endif // VK_KHR_android_surface
#if VK_EXT_metal_surface
  static std::shared_ptr<Surface> create(
    VkInstance inst,
    const VkMetalSurfaceCreateInfoEXT* msci
  ) {
    VkSurfaceKHR surf;
    VK_ASSERT << vkCreateMetalSurfaceEXT(inst, msci, nullptr, &surf);
    return std::make_shared<Surface>(inst, surf, true);
  }
#endif // VK_EXT_metal_surface
  void destroy() {
    if (should_destroy) {
      vkDestroySurfaceKHR(inst, surf, nullptr);
    }
  }
};
typedef std::shared_ptr<Surface> SurfaceRef;

struct Swapchain {
  typedef Swapchain Self;
  VkDevice dev;
  VkSwapchainKHR swapchain;
  bool should_destroy;

  Swapchain(VkDevice dev, VkSwapchainKHR swapchain, bool should_destroy) :
    dev(dev), swapchain(swapchain), should_destroy(should_destroy) {}
  ~Swapchain() {
    destroy();
  }

  operator VkSwapchainKHR() const {
    return swapchain;
  }

  static std::shared_ptr<Swapchain> create(
    VkDevice dev,
    const VkSwapchainCreateInfoKHR* sci
  ) {
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateSwapchainKHR(dev, sci, nullptr, &swapchain);
    return std::make_shared<Swapchain>(dev, swapchain, true);
  }
  void destroy() {
    if (should_destroy) {
      vkDestroySwapchainKHR(dev, swapchain, nullptr);
    }
  }
};
typedef std::shared_ptr<Swapchain> SwapchainRef;

struct ShaderModule {
  typedef ShaderModule Self;
  VkDevice dev;
  VkShaderModule shader_module;
  bool should_destroy;

  ShaderModule(
    VkDevice dev,
    VkShaderModule shader_module,
    bool should_destroy
  ) :
    dev(dev), shader_module(shader_module), should_destroy(should_destroy) {}
  ~ShaderModule() {
    destroy();
  }

  operator VkShaderModule() const {
    return shader_module;
  }

  static std::shared_ptr<ShaderModule> create(
    VkDevice dev,
    const VkShaderModuleCreateInfo* smci
  ) {
    VkShaderModule shader_module = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateShaderModule(dev, smci, nullptr, &shader_module);
    return std::make_shared<ShaderModule>(dev, shader_module, true);
  }
  void destroy() {
    if (should_destroy) {
      vkDestroyShaderModule(dev, shader_module, nullptr);
    }
  }
};
typedef std::shared_ptr<ShaderModule> ShaderModuleRef;

struct DescriptorSetLayout {
  typedef DescriptorSetLayout Self;
  VkDevice dev;
  VkDescriptorSetLayout desc_set_layout;
  bool should_destroy;

  DescriptorSetLayout(
    VkDevice dev,
    VkDescriptorSetLayout desc_set_layout,
    bool should_destroy
  ) :
    dev(dev),
    desc_set_layout(desc_set_layout),
    should_destroy(should_destroy) {}
  ~DescriptorSetLayout() {
    destroy();
  }

  operator VkDescriptorSetLayout() {
    return desc_set_layout;
  }

  static std::shared_ptr<DescriptorSetLayout> create(
    VkDevice dev,
    const VkDescriptorSetLayoutCreateInfo* dslci
  ) {
    VkDescriptorSetLayout desc_set_layout = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateDescriptorSetLayout(
      dev, dslci, nullptr, &desc_set_layout
    );
    return std::make_shared<DescriptorSetLayout>(dev, desc_set_layout, true);
  }
  void destroy() {
    if (should_destroy) {
      vkDestroyDescriptorSetLayout(dev, desc_set_layout, nullptr);
    }
  }
};
typedef std::shared_ptr<DescriptorSetLayout> DescriptorSetLayoutRef;

struct PipelineLayout {
  typedef PipelineLayout Self;
  VkDevice dev;
  VkPipelineLayout pipe_layout;
  bool should_destroy;

  PipelineLayout(
    VkDevice dev,
    VkPipelineLayout pipe_layout,
    bool should_destroy
  ) :
    dev(dev), pipe_layout(pipe_layout), should_destroy(should_destroy) {}
  ~PipelineLayout() {
    destroy();
  }

  operator VkPipelineLayout() {
    return pipe_layout;
  }

  static std::shared_ptr<PipelineLayout> create(
    VkDevice dev,
    const VkPipelineLayoutCreateInfo* plci
  ) {
    VkPipelineLayout pipe_layout = VK_NULL_HANDLE;
    VK_ASSERT << vkCreatePipelineLayout(dev, plci, nullptr, &pipe_layout);
    return std::make_shared<PipelineLayout>(dev, pipe_layout, true);
  }
  void destroy() {
    if (should_destroy) {
      vkDestroyPipelineLayout(dev, pipe_layout, nullptr);
    }
  }
};
typedef std::shared_ptr<PipelineLayout> PipelineLayoutRef;

struct Pipeline {
  typedef Pipeline Self;
  VkDevice dev;
  VkPipeline pipe;
  bool should_destroy;

  Pipeline(VkDevice dev, VkPipeline pipe, bool should_destroy) :
    dev(dev), pipe(pipe), should_destroy(should_destroy) {}
  ~Pipeline() {
    destroy();
  }

  operator VkPipeline() {
    return pipe;
  }

  static std::shared_ptr<Pipeline> create(
    VkDevice dev,
    const VkComputePipelineCreateInfo* cpci
  ) {
    VkPipeline pipe = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateComputePipelines(
      dev, VK_NULL_HANDLE, 1, cpci, nullptr, &pipe
    );
    return std::make_shared<Pipeline>(dev, pipe, true);
  }
  static std::shared_ptr<Pipeline> create(
    VkDevice dev,
    const VkGraphicsPipelineCreateInfo* gpci
  ) {
    VkPipeline pipe = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateGraphicsPipelines(
      dev, VK_NULL_HANDLE, 1, gpci, nullptr, &pipe
    );
    return std::make_shared<Pipeline>(dev, pipe, true);
  }
  void destroy() {
    if (should_destroy) {
      vkDestroyPipeline(dev, pipe, nullptr);
    }
  }
};
typedef std::shared_ptr<Pipeline> PipelineRef;

struct RenderPass {
  typedef RenderPass Self;
  VkDevice dev;
  VkRenderPass pass;
  bool should_destroy;

  RenderPass(VkDevice dev, VkRenderPass pass, bool should_destroy) :
    dev(dev), pass(pass), should_destroy(should_destroy) {}
  ~RenderPass() {
    destroy();
  }

  operator VkRenderPass() {
    return pass;
  }

  static std::shared_ptr<RenderPass> create(
    VkDevice dev,
    const VkRenderPassCreateInfo* rpci
  ) {
    VkRenderPass pass = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateRenderPass(dev, rpci, nullptr, &pass);
    return std::make_shared<RenderPass>(dev, pass, true);
  }
  void destroy() {
    if (should_destroy) {
      vkDestroyRenderPass(dev, pass, nullptr);
    }
  }
};
typedef std::shared_ptr<RenderPass> RenderPassRef;

struct Framebuffer {
  typedef Framebuffer Self;
  VkDevice dev;
  VkFramebuffer framebuf;
  bool should_destroy;

  Framebuffer(VkDevice dev, VkFramebuffer framebuf, bool should_destroy) :
    dev(dev), framebuf(framebuf), should_destroy(should_destroy) {}
  ~Framebuffer() {
    destroy();
  }

  operator VkFramebuffer() {
    return framebuf;
  }

  static std::shared_ptr<Framebuffer> create(
    VkDevice dev,
    const VkFramebufferCreateInfo* fci
  ) {
    VkFramebuffer framebuf = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateFramebuffer(dev, fci, nullptr, &framebuf);
    return std::make_shared<Framebuffer>(dev, framebuf, true);
  }
  void destroy() {
    if (should_destroy) {
      vkDestroyFramebuffer(dev, framebuf, nullptr);
    }
  }
};
typedef std::shared_ptr<Framebuffer> FramebufferRef;

struct DescriptorPool {
  typedef DescriptorPool Self;
  VkDevice dev;
  VkDescriptorPool desc_pool;
  bool should_destroy;

  DescriptorPool(
    VkDevice dev,
    VkDescriptorPool desc_pool,
    bool should_destroy
  ) :
    dev(dev), desc_pool(desc_pool), should_destroy(should_destroy) {}
  ~DescriptorPool() {
    destroy();
  }

  static std::shared_ptr<DescriptorPool> create(
    VkDevice dev,
    const VkDescriptorPoolCreateInfo* dpci
  ) {
    VkDescriptorPool desc_pool = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateDescriptorPool(dev, dpci, nullptr, &desc_pool);
    return std::make_shared<DescriptorPool>(dev, desc_pool, true);
  }
  void destroy() {
    if (should_destroy) {
      vkDestroyDescriptorPool(dev, desc_pool, nullptr);
    }
  }
};
typedef std::shared_ptr<DescriptorPool> DescriptorPoolRef;

struct DescriptorSet {
  typedef DescriptorSet Self;
  VkDescriptorSet desc_set;
  bool should_destroy;

  DescriptorSet(VkDescriptorSet desc_set, bool should_destroy) :
    desc_set(desc_set), should_destroy(should_destroy) {}
  ~DescriptorSet() {
    destroy();
  }

  static std::shared_ptr<DescriptorSet> create(
    VkDevice dev,
    const VkDescriptorSetAllocateInfo* dsai
  ) {
    VkDescriptorSet desc_set = VK_NULL_HANDLE;
    VK_ASSERT << vkAllocateDescriptorSets(dev, dsai, &desc_set);
    return std::make_shared<DescriptorSet>(desc_set, true);
  }
  void destroy() {}
};
typedef std::shared_ptr<DescriptorSet> DescriptorSetRef;

struct CommandPool {
  typedef CommandPool Self;
  VkDevice dev;
  VkCommandPool cmd_pool;
  bool should_destroy;

  CommandPool(VkDevice dev, VkCommandPool cmd_pool, bool should_destroy) :
    dev(dev), cmd_pool(cmd_pool), should_destroy(should_destroy) {}
  ~CommandPool() {
    destroy();
  }

  operator VkCommandPool() {
    return cmd_pool;
  }

  static std::shared_ptr<CommandPool> create(
    VkDevice dev,
    const VkCommandPoolCreateInfo* cpci
  ) {
    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateCommandPool(dev, cpci, nullptr, &cmd_pool);
    return std::make_shared<CommandPool>(dev, cmd_pool, true);
  }
  void destroy() {
    if (should_destroy) {
      vkDestroyCommandPool(dev, cmd_pool, nullptr);
    }
  }
};
typedef std::shared_ptr<CommandPool> CommandPoolRef;

struct CommandBuffer {
  typedef CommandBuffer Self;
  VkCommandBuffer cmdbuf;
  bool should_destroy;

  CommandBuffer(VkCommandBuffer cmdbuf, bool should_destroy) :
    cmdbuf(cmdbuf), should_destroy(should_destroy) {}
  ~CommandBuffer() {
    destroy();
  }

  operator VkCommandBuffer() {
    return cmdbuf;
  }

  static std::shared_ptr<CommandBuffer> create(
    VkDevice dev,
    const VkCommandBufferAllocateInfo* cbai
  ) {
    VkCommandBuffer cmdbuf = VK_NULL_HANDLE;
    VK_ASSERT << vkAllocateCommandBuffers(dev, cbai, &cmdbuf);
    return std::make_shared<CommandBuffer>(cmdbuf, true);
  }
  void destroy() {}
};
typedef std::shared_ptr<CommandBuffer> CommandBufferRef;

struct Fence {
  typedef Fence Self;
  VkDevice dev;
  VkFence fence;
  bool should_destroy;

  Fence(VkDevice dev, VkFence fence, bool should_destroy) :
    dev(dev), fence(fence), should_destroy(should_destroy) {}
  ~Fence() {
    destroy();
  }

  operator VkFence() const {
    return fence;
  }

  static std::shared_ptr<Fence> create(VkDevice dev) {
    VkFenceCreateInfo fci {};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    return create(dev, &fci);
  }
  static std::shared_ptr<Fence> create(
    VkDevice dev,
    const VkFenceCreateInfo* fci
  ) {
    VkFence fence = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateFence(dev, fci, nullptr, &fence);
    return std::make_shared<Fence>(dev, fence, true);
  }
  void destroy() {
    if (should_destroy) {
      vkDestroyFence(dev, fence, nullptr);
    }
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
  ~Semaphore() {
    destroy();
  }

  operator VkSemaphore() const {
    return sema;
  }

  static std::shared_ptr<Semaphore> create(VkDevice dev) {
    VkSemaphoreCreateInfo sci {};
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    return Self::create(dev, &sci);
  }
  static std::shared_ptr<Semaphore> create(
    VkDevice dev,
    const VkSemaphoreCreateInfo* sci
  ) {
    VkSemaphore sema = VK_NULL_HANDLE;
    VK_ASSERT << vkCreateSemaphore(dev, sci, nullptr, &sema);
    return std::make_shared<Semaphore>(dev, sema, true);
  }
  void destroy() {
    if (should_destroy) {
      vkDestroySemaphore(dev, sema, nullptr);
    }
  }
};
typedef std::shared_ptr<Semaphore> SemaphoreRef;

struct QueryPool {
  typedef QueryPool Self;
  VkDevice dev;
  VkQueryPool query_pool;
  bool should_destroy;

  QueryPool(VkDevice dev, VkQueryPool query_pool, bool should_destroy) :
    dev(dev), query_pool(query_pool), should_destroy(should_destroy) {}
  ~QueryPool() {
    destroy();
  }

  operator VkQueryPool() const {
    return query_pool;
  }

  static std::shared_ptr<QueryPool> create(
    VkDevice dev,
    const VkQueryPoolCreateInfo* qpci
  ) {
    VkQueryPool query_pool;
    VK_ASSERT << vkCreateQueryPool(dev, qpci, nullptr, &query_pool);
    return std::make_shared<QueryPool>(dev, query_pool, true);
  }
  void destroy() {
    if (should_destroy) {
      vkDestroyQueryPool(dev, query_pool, nullptr);
    }
  }
};
typedef std::shared_ptr<QueryPool> QueryPoolRef;

} // namespace sys
} // namespace vk
} // namespace liong
