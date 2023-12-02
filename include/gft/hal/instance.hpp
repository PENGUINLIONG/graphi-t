#pragma once
#include "gft/hal/hal.hpp"

namespace liong {
namespace hal {

struct Instance : std::enable_shared_from_this<Instance> {
  Instance() {}
  virtual ~Instance() {}

  // Generate Human-readable string to describe the properties and capabilities
  // of the device at index `idx`. If there is no device at `idx`, an empty
  // string is returned.
  virtual std::string describe_device(uint32_t device_index) = 0;

  virtual ContextRef create_context(const ContextConfig& cfg) = 0;
  virtual ContextRef create_context(const ContextWindowsConfig& cfg) = 0;
  virtual ContextRef create_context(const ContextAndroidConfig& cfg) = 0;
  virtual ContextRef create_context(const ContextMetalConfig& cfg) = 0;
};

} // namespace hal
} // namespace liong
