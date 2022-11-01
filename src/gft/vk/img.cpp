#include "gft/vk.hpp"
#include "gft/log.hpp"

namespace liong {
namespace vk {

Image create_img(const Context& ctxt, const ImageConfig& img_cfg) {
  VkFormat fmt = fmt2vk(img_cfg.fmt, img_cfg.cspace);
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
    usage |=
      VK_IMAGE_USAGE_SAMPLED_BIT |
      VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    init_submit_ty = L_SUBMIT_TYPE_ANY;
  }
  if (img_cfg.usage & L_IMAGE_USAGE_STORAGE_BIT) {
    usage |=
      VK_IMAGE_USAGE_STORAGE_BIT |
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
      VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    init_submit_ty = L_SUBMIT_TYPE_ANY;
  }
  // KEEP THIS AFTER ANY SUBMIT TYPES.
  if (img_cfg.usage & L_IMAGE_USAGE_ATTACHMENT_BIT) {
    usage |=
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    init_submit_ty = L_SUBMIT_TYPE_GRAPHICS;
  }
  if (img_cfg.usage & L_IMAGE_USAGE_SUBPASS_DATA_BIT) {
    usage |=
      VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
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
  VK_ASSERT << vkGetPhysicalDeviceImageFormatProperties(ctxt.physdev(),
    fmt, img_ty, VK_IMAGE_TILING_OPTIMAL, usage, 0, &ifp);

  VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkImageCreateInfo ici {};
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
  VmaAllocationCreateInfo aci {};
  VkResult res = VK_ERROR_OUT_OF_DEVICE_MEMORY;
  if (is_tile_mem) {
    aci.usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
    img = sys::Image::create(*ctxt.allocator, &ici, &aci);
  }
  if (res != VK_SUCCESS) {
    if (is_tile_mem) {
      L_WARN("tile-memory is unsupported, fall back to regular memory");
    }
    aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    img = sys::Image::create(*ctxt.allocator, &ici, &aci);
  }

  VkImageViewCreateInfo ivci {};
  ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ivci.image = img->img;
  ivci.viewType = img_view_ty;
  ivci.format = fmt;
  ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  ivci.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
  ivci.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

  sys::ImageViewRef img_view = sys::ImageView::create(ctxt.dev->dev, &ivci);

  ImageDynamicDetail dyn_detail {};
  dyn_detail.layout = layout;
  dyn_detail.access = 0;
  dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;

  L_DEBUG("created image '", img_cfg.label, "'");
  uint32_t qfam_idx = ctxt.submit_details.at(init_submit_ty).qfam_idx;
  return Image {
    &ctxt, std::move(img), std::move(img_view), img_cfg, std::move(dyn_detail)
  };
}
Image::~Image() {
  if (img) {
    L_DEBUG("destroyed image '", img_cfg.label, "'");
  }
}
const ImageConfig& get_img_cfg(const Image& img) {
  return img.img_cfg;
}

void map_img_mem(
  const ImageView& img,
  MemoryAccess map_access,
  void*& mapped,
  size_t& row_pitch
) {
  unimplemented();
  L_ASSERT(map_access != 0, "memory map access must be read, write or both");

  VkImageSubresource is {};
  is.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  is.arrayLayer = 0;
  is.mipLevel = 0;

  VkSubresourceLayout sl {};
  vkGetImageSubresourceLayout(img.img->ctxt->dev->dev, img.img->img->img, &is, &sl);
  size_t offset = sl.offset;
  size_t size = sl.size;

  VK_ASSERT << vmaMapMemory(*img.img->ctxt->allocator, img.img->img->alloc, &mapped);
  row_pitch = sl.rowPitch;

  auto& dyn_detail = (ImageDynamicDetail&)(img.img->dyn_detail);
  L_ASSERT(dyn_detail.layout == VK_IMAGE_LAYOUT_PREINITIALIZED,
    "linear image cannot be initialized after other use");
  dyn_detail.access = map_access == L_MEMORY_ACCESS_READ_BIT ?
    VK_ACCESS_HOST_READ_BIT : VK_ACCESS_HOST_WRITE_BIT;
  dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;

  L_DEBUG("mapped image '", img.img->img_cfg.label, "' from (", img.x_offset,
    ", ", img.y_offset, ") to (", img.x_offset + img.width, ", ",
    img.y_offset + img.height, ")");
}
void unmap_img_mem(
  const ImageView& img,
  void* mapped
) {
  unimplemented();
  vmaUnmapMemory(*img.img->ctxt->allocator, img.img->img->alloc);
  L_DEBUG("unmapped image '", img.img->img_cfg.label, "'");
}

} // namespace vk
} // namespace liong