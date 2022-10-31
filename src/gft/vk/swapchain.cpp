#include "gft/vk.hpp"
#include "gft/log.hpp"

namespace liong {
namespace vk {

sys::SwapchainRef _create_swapchain(
  const Context& ctxt,
  const SwapchainConfig& cfg,
  VkSwapchainKHR old_swapchain,
  SwapchainDynamicDetail& dyn_detail
) {
  auto physdev = ctxt.physdev();

  VkSurfaceCapabilitiesKHR sc {};
  VK_ASSERT <<
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physdev, ctxt.surf->surf, &sc);
  L_DEBUG("current surface image size is (", sc.currentExtent.width, ", ",
    sc.currentExtent.height, ")");

  uint32_t width = sc.currentExtent.width;
  uint32_t height = sc.currentExtent.height;

  uint32_t nimg = cfg.nimg;
  if (nimg < sc.minImageCount || nimg > sc.maxImageCount) {
    uint32_t nimg2 =
      std::max(std::min(nimg, sc.maxImageCount), sc.minImageCount);
    L_WARN("physical device cannot afford ", nimg, " swapchain images, "
      "fallback to ", nimg2);
  }

  uint32_t nsurf_fmt = 0;
  VK_ASSERT << vkGetPhysicalDeviceSurfaceFormatsKHR(physdev,
    ctxt.surf->surf, &nsurf_fmt, nullptr);
  std::vector<VkSurfaceFormatKHR> surf_fmts;
  surf_fmts.resize(nsurf_fmt);
  VK_ASSERT << vkGetPhysicalDeviceSurfaceFormatsKHR(physdev,
    ctxt.surf->surf, &nsurf_fmt, surf_fmts.data());

  VkColorSpaceKHR cspace = cspace2vk(cfg.cspace);
  VkFormat fmt = VK_FORMAT_UNDEFINED;
  fmt::Format selected_fmt2 = fmt::L_FORMAT_UNDEFINED;
  for (fmt::Format fmt2 : cfg.fmts) {
    VkFormat candidate_fmt = fmt2vk(fmt2, fmt::L_COLOR_SPACE_LINEAR);

    bool found = false;
    for (const VkSurfaceFormatKHR& surf_fmt : surf_fmts) {
      if (surf_fmt.format == candidate_fmt && surf_fmt.colorSpace == cspace) {
        fmt = candidate_fmt;
        selected_fmt2 = fmt2;
        found = true;
        break;
      }
    }

    if (found) {
      break;
    }
  }
  L_ASSERT(fmt != VK_FORMAT_UNDEFINED, "surface format is not supported by the underlying platform");

  VkSwapchainCreateInfoKHR sci {};
  sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  sci.surface = ctxt.surf->surf;
  sci.oldSwapchain = old_swapchain;
  sci.minImageCount = nimg;
  sci.imageFormat = fmt;
  sci.imageColorSpace = cspace;
  sci.imageExtent.width = width;
  sci.imageExtent.height = height;
  sci.imageArrayLayers = 1;
  sci.imageUsage =
    VK_IMAGE_USAGE_TRANSFER_DST_BIT |
    VK_IMAGE_USAGE_STORAGE_BIT |
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  sci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  sci.clipped = VK_TRUE;

  sys::SwapchainRef swapchain = sys::Swapchain::create(*ctxt.dev, &sci);

  // Collect swapchain images.
  std::vector<VkImage> imgs;
  {
    uint32_t nimg2;
    VK_ASSERT << vkGetSwapchainImagesKHR(ctxt.dev->dev, *swapchain, &nimg2, nullptr);
    L_ASSERT(nimg == nimg2, "expected ", nimg, "swapchain images, but actually "
      "get ", nimg2, " images");
    imgs.resize(nimg2);
    VK_ASSERT << vkGetSwapchainImagesKHR(ctxt.dev->dev, *swapchain, &nimg2, imgs.data());
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

    sys::ImageViewRef img_view = sys::ImageView::create(ctxt.dev->dev, &ivci);

    Image out {};
    out.img = std::make_shared<sys::Image>(*ctxt.allocator, img, nullptr, false);
    out.img_view = std::move(img_view);
    out.img_cfg.label = util::format(cfg.label, " #", i);
    out.img_cfg.width = width;
    out.img_cfg.height = height;
    out.img_cfg.usage =
      L_IMAGE_USAGE_ATTACHMENT_BIT |
      L_IMAGE_USAGE_PRESENT_BIT;
    out.img_cfg.fmt = selected_fmt2;
    out.dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;
    out.dyn_detail.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    out.dyn_detail.access = 0;

    swapchain_imgs.emplace_back(std::move(out));
  }

  dyn_detail = SwapchainDynamicDetail {
    width, height, std::move(swapchain_imgs), nullptr
  };

  return swapchain;
}

void _init_swapchain(Swapchain& swapchain) {
  if (swapchain.dyn_detail == nullptr) {
    SwapchainDynamicDetail dyn_detail {};
    swapchain.swapchain = _create_swapchain(*swapchain.ctxt,
      swapchain.swapchain_cfg, *swapchain.swapchain, dyn_detail);
  }

  const Context& ctxt = *swapchain.ctxt;
  SwapchainDynamicDetail& dyn_detail = *swapchain.dyn_detail;
  dyn_detail.img_idx = std::make_unique<uint32_t>(~0u);

  VkFenceCreateInfo fci {};
  fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  VkFence fence;
  VK_ASSERT << vkCreateFence(ctxt.dev->dev, &fci, nullptr, &fence);

  VkResult acq_res = vkAcquireNextImageKHR(ctxt.dev->dev, *swapchain.swapchain,
    SPIN_INTERVAL, VK_NULL_HANDLE, fence, &*dyn_detail.img_idx);
  L_ASSERT(acq_res >= 0, "failed to initiate swapchain image acquisition");

  // Ensure the first image is acquired. It shouldn't take long.
  VK_ASSERT << vkWaitForFences(ctxt.dev->dev, 1, &fence, VK_TRUE, SPIN_INTERVAL);

  vkDestroyFence(ctxt.dev->dev, fence, nullptr);
}
Swapchain create_swapchain(const Context& ctxt, const SwapchainConfig& cfg) {
  SwapchainDynamicDetail dyn_detail;
  sys::SwapchainRef swapchain =
    _create_swapchain(ctxt, cfg, VK_NULL_HANDLE, dyn_detail);

  Swapchain out {
    &ctxt, cfg, std::move(swapchain),
    std::make_unique<SwapchainDynamicDetail>(std::move(dyn_detail)),
  };
  _init_swapchain(out);
  return std::move(out);
}
Swapchain::~Swapchain() {
  L_DEBUG("destroyed swapchain '", swapchain_cfg.label, "'");
}

const Image& get_swapchain_img(const Swapchain& swapchain) {
  L_ASSERT(swapchain.dyn_detail != nullptr,
    "swapchain recreation is required; call `acquire_swapchain_img` first");

  const SwapchainDynamicDetail& dyn_detail = *swapchain.dyn_detail;
  L_ASSERT(*dyn_detail.img_idx != ~0u,
    "swapchain has not acquired an image for this frame");

  return dyn_detail.imgs[*dyn_detail.img_idx];
}
uint32_t get_swapchain_img_width(const Swapchain& swapchain) {
  return swapchain.dyn_detail->width;
}
uint32_t get_swapchain_img_height(const Swapchain& swapchain) {
  return swapchain.dyn_detail->height;
}


} // namespace vk
} // namespace liong
