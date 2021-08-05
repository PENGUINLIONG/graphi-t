#pragma once
#include "scoped-hal.hpp"

#ifndef HAL_IMPL_NAMESPACE
static_assert(false, "please specify the implementation namespace (e.g. `vk`)");
#endif

namespace liong {
namespace HAL_IMPL_NAMESPACE {
namespace scoped {

Context::Context(const ContextConfig& cfg) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Context>(create_ctxt(cfg))) {}



Buffer::Buffer(const Context& ctxt, const BufferConfig& cfg) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Buffer>(
    create_buf(*ctxt.inner, cfg))) {}



Image::Image(const Context& ctxt, const ImageConfig& cfg) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Image>(
    create_img(*ctxt.inner, cfg))) {}



Task::Task(const Context& ctxt, const ComputeTaskConfig& cfg) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Task>(
    create_comp_task(*ctxt.inner, cfg))) {}



ResourcePool::ResourcePool(const Context& ctxt, const Task& task) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::ResourcePool>(
    create_rsc_pool(*ctxt.inner, *task.inner))) {}



Transaction::Transaction(const Context& ctxt, const Command* cmds, size_t ncmd) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Transaction>(
    create_transact(*ctxt.inner, cmds, ncmd))) {}



CommandDrain::CommandDrain(const Context& ctxt) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::CommandDrain>(
    create_cmd_drain(*ctxt.inner))) {}

} // namespace scoped
} // namespace HAL_IMPL_NAMESPACE
} // namespace liong
