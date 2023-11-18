#pragma once
#include "gft/hal/image.hpp"
#include "gft/vk/vk-context.hpp"

namespace liong {
namespace vk {

struct VulkanImage;
typedef std::shared_ptr<VulkanImage> VulkanImageRef;

struct ImageDynamicDetail {
  VkPipelineStageFlags stage;
  VkAccessFlags access;
  VkImageLayout layout;
};
struct VulkanImage: public Image {
  VulkanContextRef ctxt; // Lifetime bound.
  sys::ImageRef img;
  sys::ImageViewRef img_view;
  ImageDynamicDetail dyn_detail;

  static ImageRef create(const ContextRef &ctxt, const ImageConfig &cfg);
  VulkanImage(VulkanContextRef ctxt, ImageInfo &&info);
  ~VulkanImage();

  inline static VulkanImageRef from_hal(const ImageRef &ref) {
    return std::static_pointer_cast<VulkanImage>(ref);
  }
};

inline VkFormat format2vk(fmt::Format format, fmt::ColorSpace color_space) {
  using namespace fmt;
  switch (format) {
  case L_FORMAT_R8G8B8A8_UNORM:
    if (color_space == L_COLOR_SPACE_SRGB) {
      return VK_FORMAT_R8G8B8A8_SRGB;
    } else {
      return VK_FORMAT_R8G8B8A8_UNORM;
    }
  case L_FORMAT_B8G8R8A8_UNORM:
    if (color_space == L_COLOR_SPACE_SRGB) {
      return VK_FORMAT_B8G8R8A8_SRGB;
    } else {
      return VK_FORMAT_B8G8R8A8_UNORM;
    }
  case L_FORMAT_B10G11R11_UFLOAT_PACK32: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
  case L_FORMAT_R16G16B16A16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
  case L_FORMAT_R32_SFLOAT: return VK_FORMAT_R32_SFLOAT;
  case L_FORMAT_R32G32_SFLOAT: return VK_FORMAT_R32G32_SFLOAT;
  case L_FORMAT_R32G32B32A32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
  default: panic("unrecognized pixel format");
  }
  return VK_FORMAT_UNDEFINED;
}

} // namespace vk
} // namespace liong
