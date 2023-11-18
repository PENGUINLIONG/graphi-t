#include "sys.hpp"
#include "gft/assert.hpp"
#include "gft/util.hpp"
#include "gft/log.hpp"
#include "gft/vk/vk-swapchain.hpp"

namespace liong {
namespace vk {

fmt::Format _get_valid_format(const VulkanContext &ctxt,
                              const SwapchainConfig &cfg) {
  uint32_t nsurf_fmt = 0;
  VK_ASSERT << vkGetPhysicalDeviceSurfaceFormatsKHR(ctxt.physdev(),
    ctxt.surf->surf, &nsurf_fmt, nullptr);
  std::vector<VkSurfaceFormatKHR> surf_fmts;
  surf_fmts.resize(nsurf_fmt);
  VK_ASSERT << vkGetPhysicalDeviceSurfaceFormatsKHR(ctxt.physdev(),
    ctxt.surf->surf, &nsurf_fmt, surf_fmts.data());

  VkColorSpaceKHR cspace = color_space2vk(cfg.color_space);
  VkFormat fmt = VK_FORMAT_UNDEFINED;
  fmt::Format selected_fmt2 = fmt::L_FORMAT_UNDEFINED;
  for (fmt::Format fmt2 : cfg.allowed_formats) {
    VkFormat candidate_fmt = format2vk(fmt2, fmt::L_COLOR_SPACE_LINEAR);

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

  return selected_fmt2;
}

uint32_t _get_valid_image_count(const VulkanContext &ctxt,
                                const SwapchainConfig &cfg) {
  VkSurfaceCapabilitiesKHR sc {};
  VK_ASSERT << vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctxt.physdev(),
                                                         ctxt.surf->surf, &sc);

  uint32_t nimg = cfg.image_count;
  if (nimg < sc.minImageCount || nimg > sc.maxImageCount) {
    uint32_t nimg2 =
      std::max(std::min(nimg, sc.maxImageCount), sc.minImageCount);
    L_WARN("physical device cannot afford ", nimg, " swapchain images, "
      "fallback to ", nimg2);
    nimg = nimg2;
  }

  return nimg;
}

std::unique_ptr<SwapchainDynamicDetail>
_create_swapchain_dyn_detail(const VulkanSwapchain &swapchain) {

  VkSurfaceCapabilitiesKHR sc {};
  VK_ASSERT << vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      swapchain.ctxt->physdev(), swapchain.ctxt->surf->surf, &sc);

  uint32_t width = sc.currentExtent.width;
  uint32_t height = sc.currentExtent.height;
  L_DEBUG("current surface image size is (", width, ", ", height, ")");

  std::unique_ptr<SwapchainDynamicDetail> dyn_detail =
      std::make_unique<SwapchainDynamicDetail>();
  dyn_detail->width = width;
  dyn_detail->height = height;
  dyn_detail->img_idx = nullptr;
  dyn_detail->imgs = {};

  return dyn_detail;
}

sys::SwapchainRef _create_swapchain(const VulkanSwapchain &swapchain) {
  L_ASSERT(swapchain.ctxt->surf != nullptr);

  VkSwapchainKHR old_swapchain = *swapchain.swapchain;

  VkSwapchainCreateInfoKHR sci {};
  sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  sci.surface = swapchain.ctxt->surf->surf;
  sci.oldSwapchain = old_swapchain;
  sci.minImageCount = swapchain.info.image_count;
  sci.imageFormat =
      format2vk(swapchain.info.format, swapchain.info.color_space);
  sci.imageColorSpace = color_space2vk(swapchain.info.color_space);
  sci.imageExtent.width = swapchain.dyn_detail->width;
  sci.imageExtent.height = swapchain.dyn_detail->height;
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

  sys::SwapchainRef out = sys::Swapchain::create(*swapchain.ctxt->dev, &sci);

  return out;
}

void _collect_swapchain_images(const VulkanSwapchain &swapchain,
                               SwapchainDynamicDetail &dyn_detail) {
  const SwapchainInfo& info = swapchain.info;
  const sys::DeviceRef &dev = swapchain.ctxt->dev;

  // Collect swapchain images.
  uint32_t image_count = info.image_count;
  std::vector<VkImage> imgs(image_count);

  std::vector<VulkanImageRef> swapchain_imgs{};
  for (uint32_t i = 0; i < image_count; ++i) {
    VkImage img = imgs[i];

    VkImageViewCreateInfo ivci {};
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image = img;
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = format2vk(info.format, info.color_space);
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ivci.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    ivci.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

    sys::ImageViewRef img_view = sys::ImageView::create(dev->dev, &ivci);

    VulkanImageRef out = std::make_shared<VulkanImage>(swapchain.ctxt);
    out->img = std::make_shared<sys::Image>(*swapchain.ctxt->allocator, img, nullptr, false);
    out->img_view = std::move(img_view);
    out->img_cfg.label = util::format(info.label, " #", i);
    out->img_cfg.width = dyn_detail.width;
    out->img_cfg.height = dyn_detail.height;
    out->img_cfg.usage =
      L_IMAGE_USAGE_ATTACHMENT_BIT |
      L_IMAGE_USAGE_PRESENT_BIT;
    out->img_cfg.format = info.format;
    out->dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;
    out->dyn_detail.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    out->dyn_detail.access = 0;

    swapchain_imgs.emplace_back(std::move(out));
  }

  dyn_detail.imgs = std::move(swapchain_imgs);
}
void _acquire_swapchain_img(VulkanSwapchain &swapchain) {
  const VulkanContext& ctxt = *swapchain.ctxt;
  SwapchainDynamicDetail& dyn_detail = *swapchain.dyn_detail;
  dyn_detail.img_idx = std::make_unique<uint32_t>(~0u);

  VkFenceCreateInfo fci {};
  fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  VkFence fence;
  // FIXME: (penguinliong) This is slow.
  VK_ASSERT << vkCreateFence(ctxt.dev->dev, &fci, nullptr, &fence);

  VkResult acq_res = vkAcquireNextImageKHR(ctxt.dev->dev, *swapchain.swapchain,
    SPIN_INTERVAL, VK_NULL_HANDLE, fence, &*dyn_detail.img_idx);
  L_ASSERT(acq_res >= 0, "failed to initiate swapchain image acquisition");

  // Ensure the first image is acquired. It shouldn't take long.
  VK_ASSERT << vkWaitForFences(ctxt.dev->dev, 1, &fence, VK_TRUE,
                               SPIN_INTERVAL);

  vkDestroyFence(ctxt.dev->dev, fence, nullptr);
}

void _recreate_swapchain(VulkanSwapchain &swapchain) {
  swapchain.dyn_detail = _create_swapchain_dyn_detail(swapchain);
  swapchain.swapchain = _create_swapchain(swapchain);
  _collect_swapchain_images(swapchain, *swapchain.dyn_detail);
  _acquire_swapchain_img(swapchain);

  L_DEBUG("created swapchain '", swapchain.info.label, "'");
}
SwapchainRef VulkanSwapchain::create(const ContextRef &ctxt, const SwapchainConfig &cfg) {
  VulkanContextRef ctxt_ = VulkanContext::from_hal(ctxt);

  fmt::Format format = _get_valid_format(*ctxt_, cfg);
  uint32_t image_count = _get_valid_image_count(*ctxt_, cfg);

  SwapchainInfo info{};
  info.label = cfg.label;
  info.image_count = image_count;
  info.format = format;
  info.color_space = cfg.color_space;

  VulkanSwapchainRef out = std::make_shared<VulkanSwapchain>(info);
  out->ctxt = ctxt_;
  out->swapchain = VK_NULL_HANDLE;
  out->dyn_detail = nullptr;

  out->recreate();

  return out;
}
VulkanSwapchain::~VulkanSwapchain() {
  if (swapchain) {
    L_DEBUG("destroyed swapchain '", info.label, "'");
  }
}

ImageRef VulkanSwapchain::get_current_image() {
  L_ASSERT(dyn_detail != nullptr,
    "swapchain recreation is required; call `acquire_swapchain_img` first");

  L_ASSERT(*dyn_detail->img_idx != ~0u,
    "swapchain has not acquired an image for this frame");

  ImageRef out = dyn_detail->imgs[*dyn_detail->img_idx];
  return out;
}

void VulkanSwapchain::recreate() {
  _recreate_swapchain(*this);
}


} // namespace vk
} // namespace liong
