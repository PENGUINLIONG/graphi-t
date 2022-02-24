#include "gft/vk.hpp"
#include "gft/log.hpp"

namespace liong {
namespace vk {

VkSwapchainKHR _create_swapchain(
  const Context& ctxt,
  const SwapchainConfig& cfg,
  VkSwapchainKHR old_swapchain,
  SwapchainDynamicDetail& dyn_detail
) {
  VkSurfaceCapabilitiesKHR sc {};
  VK_ASSERT <<
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctxt.physdev, ctxt.surf, &sc);
  log::debug("current surface image size is (", sc.currentExtent.width, ", ",
    sc.currentExtent.height, ")");

  uint32_t width = sc.currentExtent.width;
  uint32_t height = sc.currentExtent.height;

  uint32_t nimg = cfg.nimg;
  if (nimg < sc.minImageCount || nimg > sc.maxImageCount) {
    uint32_t nimg2 =
      std::max(std::min(nimg, sc.maxImageCount), sc.minImageCount);
    log::warn("physical device cannot afford ", nimg, " swapchain images, "
      "fallback to ", nimg2);
  }

  VkFormat fmt = fmt2vk(cfg.fmt);

  VkSwapchainCreateInfoKHR sci {};
  sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  sci.surface = ctxt.surf;
  sci.oldSwapchain = old_swapchain;
  sci.minImageCount = nimg;
  sci.imageFormat = fmt;
  sci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  sci.imageExtent.width = width;
  sci.imageExtent.height = height;
  sci.imageArrayLayers = 1;
  sci.imageUsage =
    VK_IMAGE_USAGE_TRANSFER_DST_BIT |
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  sci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  sci.compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
  sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  sci.clipped = VK_TRUE;

  VkSwapchainKHR swapchain;
  VK_ASSERT << vkCreateSwapchainKHR(ctxt.dev, &sci, nullptr, &swapchain);

  // Collect swapchain images.
  std::vector<VkImage> imgs;
  {
    uint32_t nimg2;
    VK_ASSERT << vkGetSwapchainImagesKHR(ctxt.dev, swapchain, &nimg2, nullptr);
    assert(nimg == nimg2, "expected ", nimg, "swapchain images, but actually "
      "get ", nimg2, " images");
    imgs.resize(nimg2);
    VK_ASSERT << vkGetSwapchainImagesKHR(ctxt.dev, swapchain, &nimg2, imgs.data());
  }

  std::vector<Image> swapchain_imgs;
  for (uint32_t i = 0; i < nimg; ++i) {
    VkImage img = imgs[i];

    VkImageViewCreateInfo ivci {};
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image = img;
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = fmt;
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ivci.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    ivci.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

    VkImageView img_view;
    VK_ASSERT << vkCreateImageView(ctxt.dev, &ivci, nullptr, &img_view);

    Image out {};
    out.alloc = {};
    out.img = img;
    out.img_view = img_view;
    out.img_cfg.label = util::format(cfg.label, " #", i);
    out.img_cfg.width = width;
    out.img_cfg.height = height;
    out.img_cfg.usage =
      L_IMAGE_USAGE_ATTACHMENT_BIT |
      L_IMAGE_USAGE_PRESENT_BIT;
    out.img_cfg.fmt = cfg.fmt;
    out.dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;
    out.dyn_detail.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    out.dyn_detail.access = 0;

    swapchain_imgs.emplace_back(std::move(out));
  }

  dyn_detail = SwapchainDynamicDetail {
    width, height, swapchain_imgs, nullptr
  };

  return swapchain;
}

Swapchain create_swapchain(const Context& ctxt, const SwapchainConfig& cfg) {
  SwapchainDynamicDetail dyn_detail;
  VkSwapchainKHR swapchain =
    _create_swapchain(ctxt, cfg, VK_NULL_HANDLE, dyn_detail);

  return Swapchain {
    &ctxt, cfg, swapchain,
    std::make_unique<SwapchainDynamicDetail>(std::move(dyn_detail)),
  };
}
void destroy_swapchain(Swapchain& swapchain) {
  const Context& ctxt = *swapchain.ctxt;
  if (swapchain.dyn_detail != nullptr) {
    SwapchainDynamicDetail& dyn_detail = *swapchain.dyn_detail;
    dyn_detail.img_idx = nullptr;
    for (size_t i = 0; i < dyn_detail.imgs.size(); ++i) {
      const Image& img = dyn_detail.imgs[i];
      vkDestroyImageView(ctxt.dev, img.img_view, nullptr);
    }
  }
  vkDestroySwapchainKHR(ctxt.dev, swapchain.swapchain, nullptr);
}

Transaction acquire_swapchain_img(Swapchain& swapchain) {
  if (swapchain.dyn_detail == nullptr) {
    SwapchainDynamicDetail dyn_detail {};
    swapchain.swapchain = _create_swapchain(*swapchain.ctxt,
      swapchain.swapchain_cfg, swapchain.swapchain, dyn_detail);
  }

  const Context& ctxt = *swapchain.ctxt;
  SwapchainDynamicDetail& dyn_detail = *swapchain.dyn_detail;

  assert(dyn_detail.img_idx == nullptr,
    "surface image has already been acquired");
  dyn_detail.img_idx = std::make_unique<uint32_t>(~0u);

  VkFenceCreateInfo fci {};
  fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  VkFence fence;
  VK_ASSERT << vkCreateFence(ctxt.dev, &fci, nullptr, &fence);

  VkResult acq_res = vkAcquireNextImageKHR(ctxt.dev, swapchain.swapchain, 0,
    VK_NULL_HANDLE, fence, &*dyn_detail.img_idx);
  assert(acq_res >= 0, "failed to initiate swapchain image acquisition");

  return Transaction { &ctxt, {}, fence };
}

const Image& get_swapchain_img(const Swapchain& swapchain) {
  assert(swapchain.dyn_detail != nullptr,
    "swapchain recreation is required; call `acquire_swapchain_img` first");

  const SwapchainDynamicDetail& dyn_detail = *swapchain.dyn_detail;
  assert(dyn_detail.img_idx != nullptr,
    "swapchain has not acquired an image for this frame");

  return dyn_detail.imgs[*dyn_detail.img_idx];
}

} // namespace vk
} // namespace liong
