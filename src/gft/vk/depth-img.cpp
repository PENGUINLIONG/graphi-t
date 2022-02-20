#include "gft/vk.hpp"
#include "gft/log.hpp"

namespace liong {
namespace vk {

DepthImage create_depth_img(
  const Context& ctxt,
  const DepthImageConfig& depth_img_cfg
) {
  VkFormat fmt = depth_fmt2vk(depth_img_cfg.fmt);
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
  VK_ASSERT << vkGetPhysicalDeviceImageFormatProperties(
    physdevs[ctxt.ctxt_cfg.dev_idx], fmt, VK_IMAGE_TYPE_2D,
    VK_IMAGE_TILING_OPTIMAL, usage, 0, &ifp);

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
  VkImage img;
  VmaAllocation alloc;
  VmaAllocationCreateInfo aci {};
  VkResult res = VK_ERROR_OUT_OF_DEVICE_MEMORY;
  if (is_tile_mem) {
    aci.usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
    res = vmaCreateImage(ctxt.allocator, &ici, &aci, &img, &alloc, nullptr);
  }
  if (res != VK_SUCCESS) {
    if (is_tile_mem) {
      log::warn("tile-memory is unsupported, fall back to regular memory");
    }
    aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    VK_ASSERT <<
      vmaCreateImage(ctxt.allocator, &ici, &aci, &img, &alloc, nullptr);
  }

  VkImageViewCreateInfo ivci {};
  ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ivci.image = img;
  ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
  ivci.format = fmt;
  ivci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  ivci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  ivci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  ivci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  ivci.subresourceRange.aspectMask =
    (fmt::get_fmt_depth_nbit(depth_img_cfg.fmt) > 0 ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
    (fmt::get_fmt_stencil_nbit(depth_img_cfg.fmt) > 0 ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
  ivci.subresourceRange.baseArrayLayer = 0;
  ivci.subresourceRange.layerCount = 1;
  ivci.subresourceRange.baseMipLevel = 0;
  ivci.subresourceRange.levelCount = 1;

  VkImageView img_view;
  VK_ASSERT << vkCreateImageView(ctxt.dev, &ivci, nullptr, &img_view);

  DepthImageDynamicDetail dyn_detail {};
  dyn_detail.layout = layout;
  dyn_detail.access = 0;
  dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;

  log::debug("created depth image '", depth_img_cfg.label, "'");
  return DepthImage {
    &ctxt, alloc, img, img_view, depth_img_cfg, std::move(dyn_detail)
  };
}
void destroy_depth_img(DepthImage& depth_img) {
  if (depth_img.img) {
    vkDestroyImageView(depth_img.ctxt->dev, depth_img.img_view, nullptr);
    vmaDestroyImage(depth_img.ctxt->allocator, depth_img.img, depth_img.alloc);

    log::debug("destroyed depth image '", depth_img.depth_img_cfg.label, "'");
    depth_img = {};
  }
}
const DepthImageConfig& get_depth_img_cfg(const DepthImage& depth_img) {
  return depth_img.depth_img_cfg;
}

} // namespace vk
} // namespace liong
