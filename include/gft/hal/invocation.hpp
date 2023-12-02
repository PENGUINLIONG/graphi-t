#pragma once
#include "gft/hal/hal.hpp"

namespace liong {
namespace hal {

struct Invocation;
typedef std::shared_ptr<Invocation> InvocationRef;

struct InvocationInfo {
  std::string label;
  // Submit type of this invocation or the first non-any subinvocation.
  SubmitType submit_ty;
};
struct Invocation : std::enable_shared_from_this<Invocation> {
  const InvocationInfo info;

  Invocation(InvocationInfo&& info) : info(std::move(info)) {}
  virtual ~Invocation() {}

  // Submit the invocation to device for execution and create a transaction for
  // the user to track the execution status.
  virtual TransactionRef create_transact(const TransactionConfig& cfg) = 0;

  // Get the execution time of the last WAITED invocation.
  virtual double get_time_us() = 0;
  // Pre-encode the invocation commands to reduce host-side overhead on constant
  // device-side procedures.
  virtual void bake() = 0;
};

} // namespace hal
} // namespace liong
