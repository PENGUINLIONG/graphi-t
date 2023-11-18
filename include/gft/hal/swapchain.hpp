#pragma once
#include "gft/hal/hal.hpp"

namespace liong {
namespace hal {

struct SwapchainInfo {
  std::string label;
  uint32_t image_count;
  fmt::Format format;
  fmt::ColorSpace color_space;
};
struct Swapchain : public std::enable_shared_from_this<Swapchain> {
  const SwapchainInfo info;

  Swapchain(SwapchainInfo&& info) : info(std::move(info)) {}
  virtual ~Swapchain() {}

  virtual const ImageRef& get_img() const = 0;
};

} // namespace hal
} // namespace liong
