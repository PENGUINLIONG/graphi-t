#pragma once
#include "gft/hal/depth-image.hpp"
#include "gft/vk/vk-context.hpp"

namespace liong {
namespace vk {

struct VulkanDepthImage;
typedef std::shared_ptr<VulkanDepthImage> VulkanDepthImageRef;

struct DepthImageDynamicDetail {
  VkPipelineStageFlags stage;
  VkAccessFlags access;
  VkImageLayout layout;
};
struct VulkanDepthImage : public DepthImage {
  VulkanContextRef ctxt;
  sys::ImageRef img;
  sys::ImageViewRef img_view;
  DepthImageDynamicDetail dyn_detail;

  static DepthImageRef create(
    const ContextRef& ctxt,
    const DepthImageConfig& cfg
  );
  VulkanDepthImage(VulkanContextRef ctxt, DepthImageInfo&& info);
  ~VulkanDepthImage();

  inline static VulkanDepthImageRef from_hal(const DepthImageRef& ref) {
    return std::static_pointer_cast<VulkanDepthImage>(ref);
  }
};

inline VkFormat depth_format2vk(fmt::DepthFormat fmt) {
  using namespace fmt;
  switch (fmt) {
  case L_DEPTH_FORMAT_D16_UNORM:
    return VK_FORMAT_D16_UNORM;
  case L_DEPTH_FORMAT_D32_SFLOAT:
    return VK_FORMAT_D32_SFLOAT;
  default:
    panic("unsupported depth format");
  }
  return VK_FORMAT_UNDEFINED;
}

} // namespace vk
} // namespace liong
