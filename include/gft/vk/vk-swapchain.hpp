#pragma once
#include "gft/hal/swapchain.hpp"
#include "gft/vk/vk-context.hpp"
#include "gft/vk/vk-image.hpp"

namespace liong {
namespace vk {

struct VulkanSwapchain;
typedef std::shared_ptr<VulkanSwapchain> VulkanSwapchainRef;

struct SwapchainDynamicDetail {
  uint32_t width;
  uint32_t height;
  std::vector<VulkanImageRef> imgs;
  std::unique_ptr<uint32_t> img_idx;
};
struct VulkanSwapchain : public Swapchain {
  VulkanContextRef ctxt;
  sys::SwapchainRef swapchain;
  std::unique_ptr<SwapchainDynamicDetail> dyn_detail;

  static SwapchainRef create(
    const ContextRef& ctxt,
    const SwapchainConfig& cfg
  );
  VulkanSwapchain(VulkanContextRef ctxt, SwapchainInfo&& info);
  ~VulkanSwapchain();

  ImageRef get_current_image();
  uint32_t get_width() const;
  uint32_t get_height() const;

  void recreate();

  inline static VulkanSwapchainRef from_hal(const SwapchainRef& ref) {
    return std::static_pointer_cast<VulkanSwapchain>(ref);
  }

  InvocationRef create_present_invocation(const PresentInvocationConfig& cfg);
};

inline VkColorSpaceKHR color_space2vk(fmt::ColorSpace cspace) {
  using namespace fmt;
  switch (cspace) {
  case L_COLOR_SPACE_SRGB:
    return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  default:
    panic("unsupported color space");
  }
}

} // namespace vk
} // namespace liong
