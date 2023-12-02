#include "gft/hal/builder.hpp"
#include "gft/hal/hal.hpp"

namespace liong {
namespace hal {

InstanceConfigBuilder InstanceConfig::build() {
  return {};
}

ContextConfigBuilder ContextConfig::build() {
  return {};
}
ContextWindowsConfigBuilder ContextWindowsConfig::build() {
  return {};
}
ContextAndroidConfigBuilder ContextAndroidConfig::build() {
  return {};
}
ContextMetalConfigBuilder ContextMetalConfig::build() {
  return {};
}

BufferConfigBuilder BufferConfig::build() {
  return {};
}

ImageConfigBuilder ImageConfig::build() {
  return {};
}

DepthImageConfigBuilder DepthImageConfig::build() {
  return {};
}

SwapchainConfigBuilder SwapchainConfig::build() {
  return {};
}

ComputeTaskConfigBuilder ComputeTaskConfig::build() {
  return {};
}

GraphicsTaskConfigBuilder GraphicsTaskConfig::build() {
  return {};
}

AttachmentConfigBuilder AttachmentConfig::build() {
  return {};
}
RenderPassConfigBuilder RenderPassConfig::build() {
  return {};
}

TransferInvocationConfigBuilder TransferInvocationConfig::build() {
  return {};
}
ComputeInvocationConfigBuilder ComputeInvocationConfig::build() {
  return {};
}
GraphicsInvocationConfigBuilder GraphicsInvocationConfig::build() {
  return {};
}
RenderPassInvocationConfigBuilder RenderPassInvocationConfig::build() {
  return {};
}
CompositeInvocationConfigBuilder CompositeInvocationConfig::build() {
  return {};
}
PresentInvocationConfigBuilder PresentInvocationConfig::build() {
  return {};
}

TransactionConfigBuilder TransactionConfig::build() {
  return {};
}

}  // namespace hal
}  // namespace liong
