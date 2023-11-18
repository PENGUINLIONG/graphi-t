#pragma once
#include "gft/hal/hal.hpp"

namespace liong {
namespace hal {

struct ImageInfo {
  std::string label;
  uint32_t width;
  uint32_t height;
  uint32_t depth;
  fmt::Format format;
  fmt::ColorSpace color_space;
  ImageUsage usage;
};
struct Image : public std::enable_shared_from_this<Image> {
  const ImageInfo info;

  Image(ImageInfo&& info) : info(std::move(info)) {}
  virtual ~Image() {}

  inline ImageView view(uint32_t x_offset, uint32_t y_offset, uint32_t z_offset,
                        uint32_t width, uint32_t height, uint32_t depth,
                        ImageSampler sampler) {
    ImageView out{};
    out.img = shared_from_this();
    out.x_offset = x_offset;
    out.y_offset = y_offset;
    out.z_offset = z_offset;
    out.width = width;
    out.height = height;
    out.depth = depth;
    out.sampler = sampler;
    return out;
  }
  inline ImageView view(
    uint32_t x_offset,
    uint32_t y_offset,
    uint32_t z_offset,
    uint32_t width,
    uint32_t height,
    uint32_t depth
  ) {
    return view(x_offset, y_offset, z_offset, width, height, depth,
      L_IMAGE_SAMPLER_LINEAR);
  }
  inline ImageView view(ImageSampler sampler) {
    return view(0, 0, 0, info.width, info.height, info.depth, sampler);
  }
  inline ImageView view() {
    return view(L_IMAGE_SAMPLER_LINEAR);
  }
};

} // namespace hal
} // namespace liong
