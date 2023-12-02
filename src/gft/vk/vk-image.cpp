#include "sys.hpp"
#include "gft/assert.hpp"
#include "gft/util.hpp"
#include "gft/log.hpp"
#include "gft/vk/vk-image.hpp"

namespace liong {
namespace vk {

VulkanImage::VulkanImage(VulkanContextRef ctxt, ImageInfo&& info) :
  Image(std::move(info)), ctxt(ctxt) {}

ImageRef VulkanImage::create(
  const ContextRef& ctxt,
  const ImageConfig& img_cfg
) {
  VulkanContextRef ctxt_ = VulkanContext::from_hal(ctxt);

  VkFormat fmt = format2vk(img_cfg.format, img_cfg.color_space);
  VkImageUsageFlags usage = 0;
  SubmitType init_submit_ty = L_SUBMIT_TYPE_ANY;

  if (img_cfg.usage & L_IMAGE_USAGE_TRANSFER_SRC_BIT) {
    usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    init_submit_ty = L_SUBMIT_TYPE_ANY;
  }
  if (img_cfg.usage & L_IMAGE_USAGE_TRANSFER_DST_BIT) {
    usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    init_submit_ty = L_SUBMIT_TYPE_ANY;
  }
  if (img_cfg.usage & L_IMAGE_USAGE_SAMPLED_BIT) {
    usage |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    init_submit_ty = L_SUBMIT_TYPE_ANY;
  }
  if (img_cfg.usage & L_IMAGE_USAGE_STORAGE_BIT) {
    usage |= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
             VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    init_submit_ty = L_SUBMIT_TYPE_ANY;
  }
  // KEEP THIS AFTER ANY SUBMIT TYPES.
  if (img_cfg.usage & L_IMAGE_USAGE_ATTACHMENT_BIT) {
    usage |=
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    init_submit_ty = L_SUBMIT_TYPE_GRAPHICS;
  }
  if (img_cfg.usage & L_IMAGE_USAGE_SUBPASS_DATA_BIT) {
    usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
             VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    init_submit_ty = L_SUBMIT_TYPE_GRAPHICS;
  }

  VkImageType img_ty;
  VkImageViewType img_view_ty;
  if (img_cfg.depth != 0) {
    img_ty = VK_IMAGE_TYPE_3D;
    img_view_ty = VK_IMAGE_VIEW_TYPE_3D;
  } else if (img_cfg.height != 0) {
    img_ty = VK_IMAGE_TYPE_2D;
    img_view_ty = VK_IMAGE_VIEW_TYPE_2D;
  } else {
    img_ty = VK_IMAGE_TYPE_1D;
    img_view_ty = VK_IMAGE_VIEW_TYPE_1D;
  }

  // Check whether the device support our use case.
  VkImageFormatProperties ifp;
  VK_ASSERT << vkGetPhysicalDeviceImageFormatProperties(
    ctxt_->physdev(), fmt, img_ty, VK_IMAGE_TILING_OPTIMAL, usage, 0, &ifp
  );

  VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkImageCreateInfo ici{};
  ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ici.imageType = img_ty;
  ici.format = fmt;
  ici.extent.width = img_cfg.width;
  ici.extent.height = img_cfg.height == 0 ? 1 : img_cfg.height;
  ici.extent.depth = img_cfg.depth == 0 ? 1 : img_cfg.depth;
  ici.mipLevels = 1;
  ici.arrayLayers = 1;
  ici.samples = VK_SAMPLE_COUNT_1_BIT;
  ici.tiling = VK_IMAGE_TILING_OPTIMAL;
  ici.usage = usage;
  ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  ici.initialLayout = layout;

  bool is_tile_mem = img_cfg.usage & L_IMAGE_USAGE_TILE_MEMORY_BIT;
  sys::ImageRef img = nullptr;
  VmaAllocationCreateInfo aci{};
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

  VkImageViewCreateInfo ivci{};
  ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ivci.image = img->img;
  ivci.viewType = img_view_ty;
  ivci.format = fmt;
  ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  ivci.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
  ivci.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

  sys::ImageViewRef img_view = sys::ImageView::create(ctxt_->dev->dev, &ivci);

  ImageDynamicDetail dyn_detail{};
  dyn_detail.layout = layout;
  dyn_detail.access = 0;
  dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;

  ImageInfo info{};
  info.label = img_cfg.label;
  info.width = img_cfg.width;
  info.height = img_cfg.height;
  info.depth = img_cfg.depth;
  info.format = img_cfg.format;
  info.color_space = img_cfg.color_space;
  info.usage = img_cfg.usage;

  VulkanImageRef out = std::make_shared<VulkanImage>(ctxt_, std::move(info));
  out->img = std::move(img);
  out->img_view = std::move(img_view);
  out->dyn_detail = std::move(dyn_detail);

  L_DEBUG("created image '", img_cfg.label, "'");

  return out;
}
VulkanImage::~VulkanImage() {
  if (img) {
    L_DEBUG("destroyed image '", info.label, "'");
  }
}

}  // namespace vk
}  // namespace liong