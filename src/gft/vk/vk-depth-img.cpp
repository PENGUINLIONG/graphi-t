#include "sys.hpp"
#include "gft/assert.hpp"
#include "gft/util.hpp"
#include "gft/log.hpp"
#include "gft/vk/vk-depth-image.hpp"

namespace liong {
namespace vk {

DepthImageRef VulkanDepthImage::create(const ContextRef &ctxt,
                                       const DepthImageConfig &depth_img_cfg) {
  VulkanContextRef ctxt_ = VulkanContext::from_hal(ctxt);

  VkFormat fmt = depth_format2vk(depth_img_cfg.depth_format);
  VkImageUsageFlags usage = 0;
  SubmitType init_submit_ty = L_SUBMIT_TYPE_ANY;

  if (depth_img_cfg.usage & L_DEPTH_IMAGE_USAGE_SAMPLED_BIT) {
    usage |=
      VK_IMAGE_USAGE_SAMPLED_BIT |
      VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    init_submit_ty = L_SUBMIT_TYPE_ANY;
  }
  if (depth_img_cfg.usage & L_DEPTH_IMAGE_USAGE_ATTACHMENT_BIT) {
    usage |=
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    init_submit_ty = L_SUBMIT_TYPE_GRAPHICS;
  }
  // KEEP THIS AFTER ANY SUBMIT TYPES.
  if (depth_img_cfg.usage & L_DEPTH_IMAGE_USAGE_SUBPASS_DATA_BIT) {
    usage |=
      VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
      VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    init_submit_ty = L_SUBMIT_TYPE_GRAPHICS;
  }

  // Check whether the device support our use case.
  VkImageFormatProperties ifp;
  VK_ASSERT << vkGetPhysicalDeviceImageFormatProperties(ctxt_->physdev(), fmt,
    VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &ifp);

  VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkImageCreateInfo ici {};
  ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ici.imageType = VK_IMAGE_TYPE_2D;
  ici.format = fmt;
  ici.extent.width = (uint32_t)depth_img_cfg.width;
  ici.extent.height = (uint32_t)depth_img_cfg.height;
  ici.extent.depth = 1;
  ici.mipLevels = 1;
  ici.arrayLayers = 1;
  ici.samples = VK_SAMPLE_COUNT_1_BIT;
  ici.tiling = VK_IMAGE_TILING_OPTIMAL;
  ici.usage = usage;
  ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  ici.initialLayout = layout;

  bool is_tile_mem = depth_img_cfg.usage & L_DEPTH_IMAGE_USAGE_TILE_MEMORY_BIT;
  sys::ImageRef img;
  VmaAllocationCreateInfo aci {};
  VkResult res = VK_ERROR_OUT_OF_DEVICE_MEMORY;
  if (is_tile_mem) {
    aci.usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
    img = sys::Image::create(*ctxt_->allocator, &ici, &aci);
  }
  if (res != VK_SUCCESS) {
    if (is_tile_mem) {
      L_WARN("tile-memory is unsupported, fall back to regular memory");
    }
    aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    img = sys::Image::create(*ctxt_->allocator, &ici, &aci);
  }

  VkImageViewCreateInfo ivci {};
  ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ivci.image = img->img;
  ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
  ivci.format = fmt;
  ivci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  ivci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  ivci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  ivci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  ivci.subresourceRange.aspectMask =
    (fmt::get_fmt_depth_nbit(depth_img_cfg.depth_format) > 0 ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
    (fmt::get_fmt_stencil_nbit(depth_img_cfg.depth_format) > 0 ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
  ivci.subresourceRange.baseArrayLayer = 0;
  ivci.subresourceRange.layerCount = 1;
  ivci.subresourceRange.baseMipLevel = 0;
  ivci.subresourceRange.levelCount = 1;

  sys::ImageViewRef img_view = sys::ImageView::create(*ctxt_->dev, &ivci);

  DepthImageDynamicDetail dyn_detail {};
  dyn_detail.layout = layout;
  dyn_detail.access = 0;
  dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;

  DepthImageInfo info{};
  info.label = depth_img_cfg.label;
  info.width = depth_img_cfg.width;
  info.height = depth_img_cfg.height;
  info.depth_format = depth_img_cfg.depth_format;
  info.usage = depth_img_cfg.usage;

  VulkanDepthImageRef out = std::make_shared<VulkanDepthImage>(ctxt_, std::move(info));
  out->img = std::move(img);
  out->img_view = std::move(img_view);
  out->dyn_detail = std::move(dyn_detail);

  L_DEBUG("created depth image '", depth_img_cfg.label, "'");

  return out;
}
VulkanDepthImage::VulkanDepthImage(VulkanContextRef ctxt, DepthImageInfo &&info)
    : DepthImage(std::move(info)), ctxt(ctxt) {}
VulkanDepthImage::~VulkanDepthImage() {
  if (img) {
    L_DEBUG("destroyed depth image '", info.label, "'");
  }
}

} // namespace vk
} // namespace liong
