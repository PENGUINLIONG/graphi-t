#pragma once
#include "gft/hal/hal.hpp"

namespace liong {
namespace hal {

struct DepthImageInfo {
  std::string label;
  uint32_t width;
  uint32_t height;
  fmt::DepthFormat depth_format;
  DepthImageUsage usage;
};
struct DepthImage : public std::enable_shared_from_this<DepthImage> {
  const DepthImageInfo info;

  DepthImage(DepthImageInfo&& info) : info(std::move(info)) {}
  virtual ~DepthImage() {}

  virtual DepthImageView view(uint32_t x_offset, uint32_t y_offset,
                              uint32_t width, uint32_t height,
                              DepthImageSampler sampler) {
    DepthImageView out{};
    out.depth_img = shared_from_this();
    out.x_offset = x_offset;
    out.y_offset = y_offset;
    out.width = width;
    out.height = height;
    out.sampler = sampler;
    return out;
  }
  inline DepthImageView view(
    uint32_t x_offset,
    uint32_t y_offset,
    uint32_t width,
    uint32_t height
  ) {
    return view(x_offset, y_offset, width, height,
      L_DEPTH_IMAGE_SAMPLER_LINEAR);
  }
  inline DepthImageView view(DepthImageSampler sampler) {
    return view(0, 0, info.width, info.height, sampler);
  }
  inline DepthImageView view() {
    return view(L_DEPTH_IMAGE_SAMPLER_LINEAR);
  }
};

} // namespace hal
} // namespace liong
