#pragma once
#include "gft/hal/hal.hpp"

namespace liong {
namespace hal {

struct ContextInfo {
  std::string label;
  uint32_t device_index;
};
struct Context : public std::enable_shared_from_this<Context> {
  const ContextInfo info;

  Context(ContextInfo&& info) : info(std::move(info)) {}
  virtual ~Context() {}

  virtual BufferRef create_buffer(const BufferConfig& cfg) = 0;
  virtual ImageRef create_image(const ImageConfig& cfg) = 0;
  virtual DepthImageRef create_depth_image(const DepthImageConfig& cfg) = 0;

  virtual SwapchainRef create_swapchain(const SwapchainConfig& cfg) = 0;
  virtual RenderPassRef create_render_pass(const RenderPassConfig& cfg) = 0;

  virtual TaskRef create_compute_task(const ComputeTaskConfig& cfg) = 0;

  virtual InvocationRef create_transfer_invocation(const TransferInvocationConfig& cfg) = 0;
  virtual InvocationRef create_composite_invocation(const CompositeInvocationConfig& cfg) = 0;
};

} // namespace hal
} // namespace liong
