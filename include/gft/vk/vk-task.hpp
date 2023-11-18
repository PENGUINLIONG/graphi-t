#pragma once
#include "gft/hal/task.hpp"
#include "gft/vk/vk-context.hpp"
#include "gft/vk/vk-render-pass.hpp"

namespace liong {
namespace vk {

struct VulkanTask;
typedef std::shared_ptr<VulkanTask> VulkanTaskRef;

struct TaskResourceDetail {
  sys::PipelineLayoutRef pipe_layout;
  std::vector<ResourceType> rsc_tys;
};
struct VulkanTask : public Task {
  VulkanContextRef ctxt;
  VulkanRenderPassRef pass; // Only for graphics task.

  sys::PipelineRef pipe;
  DispatchSize workgrp_size; // Only for compute task.
  TaskResourceDetail rsc_detail;

  static TaskRef create(const ContextRef &ctxt, const ComputeTaskConfig &cfg);
  static TaskRef create(const RenderPassRef &pass,
                        const GraphicsTaskConfig &cfg);

  VulkanTask(const ContextRef &ctxt, TaskInfo &&info);
  VulkanTask(const RenderPassRef &pass, TaskInfo &&info);
  ~VulkanTask();

  inline static VulkanTaskRef from_hal(const TaskRef &ref) {
    return std::static_pointer_cast<VulkanTask>(ref);
  }
};

} // namespace liong
} // namespace vk
