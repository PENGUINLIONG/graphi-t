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
};

} // namespace hal
} // namespace liong
