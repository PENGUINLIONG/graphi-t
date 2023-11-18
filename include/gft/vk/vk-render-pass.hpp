#pragma once
#include "gft/pool.hpp"
#include "gft/hal/render-pass.hpp"
#include "gft/vk/vk-context.hpp"

namespace liong {
namespace vk {

struct VulkanRenderPass;
typedef std::shared_ptr<VulkanRenderPass> VulkanRenderPassRef;

struct FramebufferKey {
  std::string inner;

  static FramebufferKey create(
    const VulkanRenderPass& pass,
    const std::vector<ResourceView>& rsc_views);

  friend inline bool operator<(const FramebufferKey& a, const FramebufferKey& b) {
    return a.inner < b.inner;
  }
};

typedef pool::Pool<FramebufferKey, sys::FramebufferRef> FramebufferPool;
typedef pool::PoolItem<FramebufferKey, sys::FramebufferRef> FramebufferPoolItem;

struct VulkanRenderPass : public RenderPass {
  VulkanContextRef ctxt;

  sys::RenderPassRef pass;
  std::vector<VkClearValue> clear_values;

  static RenderPassRef create(const ContextRef &ctxt, const RenderPassConfig &cfg);
  VulkanRenderPass(const VulkanContextRef& ctxt, RenderPassInfo &&info);
  ~VulkanRenderPass();

  inline static VulkanRenderPassRef from_hal(const RenderPassRef &ref) {
    return std::static_pointer_cast<VulkanRenderPass>(ref);
  }

  FramebufferPool framebuf_pool;
  FramebufferPoolItem acquire_framebuf(const std::vector<ResourceView>& attms);
};

} // namespace liong
} // namespace vk
