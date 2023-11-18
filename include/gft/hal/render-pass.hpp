#pragma once
#include "gft/hal/hal.hpp"
#include "gft/fmt.hpp"

namespace liong {
namespace hal {

struct RenderPassInfo {
  std::string label;
  uint32_t width;
  uint32_t height;
  size_t attm_count;
};
struct RenderPass : public std::enable_shared_from_this<RenderPass> {
  const RenderPassInfo info;

  RenderPass(RenderPassInfo &&info) : info(std::move(info)) {}
  virtual ~RenderPass() {}
};

} // namespace hal
} // namespace liong
