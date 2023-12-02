#pragma once
#include "gft/hal/context.hpp"

namespace liong {
namespace hal {

struct Task;
typedef std::shared_ptr<Task> TaskRef;

struct TaskInfo {
  std::string label;
  SubmitType submit_ty;
};
struct Task : std::enable_shared_from_this<Task> {
  const TaskInfo info;

  Task(TaskInfo&& info) : info(std::move(info)) {}
  virtual ~Task() {}

  virtual InvocationRef create_graphics_invocation(
    const GraphicsInvocationConfig& cfg
  ) = 0;
  virtual InvocationRef create_compute_invocation(
    const ComputeInvocationConfig& cfg
  ) = 0;
};

} // namespace hal
} // namespace liong
