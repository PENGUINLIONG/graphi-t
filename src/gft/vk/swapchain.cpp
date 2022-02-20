#include "gft/vk.hpp"
#include "gft/log.hpp"

namespace liong {
namespace vk {

VkSwapchainKHR _create_swapchain(
  const Context& ctxt,
  const SwapchainConfig& cfg
) {
  VkSwapchainCreateInfoKHR sci {};
  sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  sci.surface = surf;
  sci.minImageCount = cfg.nimg;
  sci.imageFormat = fmt2vk(cfg.fmt);
  sci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  sci.imageExtent.width = cfg.width;
  sci.imageExtent.height = cfg.height;
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

  return swapchain;
}
std::vector<Image> collect_swapchain_imgs(
  const Context& ctxt,
  VkSwapchainKHR swapchain,
  const SwapchainConfig& cfg
) {
  uint32_t nimg;
  VK_ASSERT << vkGetSwapchainImagesKHR(ctxt.dev, swapchain, &nimg, nullptr);
  std::vector<VkImage> imgs;
  imgs.resize(nimg);
  VK_ASSERT << vkGetSwapchainImagesKHR(ctxt.dev, swapchain, &nimg, imgs.data());

  std::vector<Image> swapchain_imgs;
  for (uint32_t i = 0; i < nimg; ++i) {
    VkImage img = imgs[i];

    VkImageViewCreateInfo ivci {};
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image = img;
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = fmt2vk(cfg.fmt);
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ivci.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    ivci.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

    VkImageView img_view;
    VK_ASSERT << vkCreateImageView(ctxt.dev, &ivci, nullptr, &img_view);

    Image out {};
    out.alloc = VMA_NULL;
    out.img = img;
    out.img_view = img_view;
    out.img_cfg.label = util::format(cfg.label, " #", i);
    out.img_cfg.width = cfg.width;
    out.img_cfg.height = cfg.height;
    out.img_cfg.usage =
      L_IMAGE_USAGE_ATTACHMENT_BIT |
      L_IMAGE_USAGE_PRESENT_BIT;
    out.img_cfg.fmt = cfg.fmt;
    out.dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;
    out.dyn_detail.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    out.dyn_detail.access = 0;

    swapchain_imgs.emplace_back(std::move(out));
  }

  return swapchain_imgs;
}
Swapchain create_swapchain(const Surface& surf, const SwapchainConfig& cfg) {
  const Context& ctxt = *surf.ctxt;

  VkSwapchainKHR swapchain = _create_swapchain(ctxt, cfg);
  std::vector<Image> swapchain_imgs =
    collect_swapchain_imgs(ctxt, swapchain, cfg);

  return Swapchain { &surf, swapchain, swapchain_imgs, cfg, nullptr };
}
void destroy_swapchain(Swapchain& swapchain) {
  const Context& ctxt = *swapchain.ctxt;
  for (size_t i = 0; i < surf.imgs.size(); ++i) {
    const Image& img = surf.imgs[i];
    vkDestroyImageView(ctxt.dev, img.img_view, nullptr);
  }
  vkDestroySwapchainKHR(ctxt.dev, surf.swapchain, nullptr);
}

Transaction acquire_swapchain_img(Swapchain& swapchain) {
  const Context& ctxt = *swapchain.surf->ctxt;

  assert(surf.img_idx == nullptr, "surface image has already been acquired");
  surf.img_idx = std::make_unique<uint32_t>(~0u);

  VkFenceCreateInfo fci {};
  fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  VkFence fence;
  VK_ASSERT << vkCreateFence(ctxt.dev, &fci, nullptr, &fence);

  VkResult acq_res = vkAcquireNextImageKHR(ctxt.dev, surf.swapchain, 0,
    VK_NULL_HANDLE, fence, &*surf.img_idx);
  assert(acq_res >= 0, "failed to initiate swapchain image acquisition");

  return Transaction { &ctxt, {}, fence };
}

const Image& get_swapchain_img(const Swapchain& swapchain) {
  assert(swapchain.img_idx,
    "swapchain has not acquired an image for this frame");
  return swapchain.imgs[*swapchain.img_idx];
}

} // namespace vk
} // namespace liong
