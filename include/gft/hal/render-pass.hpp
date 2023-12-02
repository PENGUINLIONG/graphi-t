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

  RenderPass(RenderPassInfo&& info) : info(std::move(info)) {}
  virtual ~RenderPass() {}

  virtual TaskRef create_graphics_task(const GraphicsTaskConfig& cfg) = 0;
  virtual InvocationRef create_render_pass_invocation(
    const RenderPassInvocationConfig& cfg
  ) = 0;
};

} // namespace hal
} // namespace liong
