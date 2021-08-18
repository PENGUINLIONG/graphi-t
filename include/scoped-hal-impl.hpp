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
Context::Context(HAL_IMPL_NAMESPACE::Context&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Context>(inner)) {}
Context::Context(const std::string& label, uint32_t dev_idx) :
  Context(ContextConfig { label, dev_idx }) {}
Context::~Context() { HAL_IMPL_NAMESPACE::destroy_ctxt(*inner); }

Task Context::create_comp_task(
  const std::string& label,
  const std::string& entry_point,
  const void* code,
  const size_t code_size,
  const std::vector<ResourceConfig>& rsc_cfgs,
  const DispatchSize& workgrp_size
) const {
  ComputeTaskConfig comp_task_cfg {};
  comp_task_cfg.label = label;
  comp_task_cfg.entry_name = entry_point.c_str();
  comp_task_cfg.code = code;
  comp_task_cfg.code_size = code_size;
  comp_task_cfg.rsc_cfgs = rsc_cfgs.data();
  comp_task_cfg.nrsc_cfg = rsc_cfgs.size();
  comp_task_cfg.workgrp_size = workgrp_size;
  return HAL_IMPL_NAMESPACE::create_comp_task(*inner, comp_task_cfg);
}

Buffer Context::create_buf(
  const std::string& label,
  MemoryAccess host_access,
  MemoryAccess dev_access,
  size_t size,
  size_t align,
  bool is_const
) const {
  BufferConfig buf_cfg {};
  buf_cfg.label = label;
  buf_cfg.size = size;
  buf_cfg.align = align;
  buf_cfg.host_access = host_access;
  buf_cfg.dev_access = dev_access;
  buf_cfg.is_const = is_const;
  return HAL_IMPL_NAMESPACE::create_buf(*inner, buf_cfg);
}
Buffer Context::create_const_buf(
  const std::string& label,
  MemoryAccess host_access,
  MemoryAccess dev_access,
  size_t size,
  size_t align
) const {
  return create_buf(label, host_access, dev_access, size, align, true);
}
Buffer Context::create_storage_buf(
  const std::string& label,
  MemoryAccess host_access,
  MemoryAccess dev_access,
  size_t size,
  size_t align
) const {
  return create_buf(label, host_access, dev_access, size, align, false);
}

Image Context::create_img(
  const std::string& label,
  MemoryAccess host_access,
  MemoryAccess dev_access,
  size_t nrow,
  size_t ncol,
  PixelFormat fmt,
  bool is_const
) const {
  size_t align =
    inner->physdev_prop.limits.optimalBufferCopyRowPitchAlignment;
  size_t pitch = align_size(ncol, align);

  ImageConfig img_cfg {};
  img_cfg.label = label;
  img_cfg.host_access = host_access;
  img_cfg.dev_access = dev_access;
  img_cfg.nrow = nrow;
  img_cfg.ncol = ncol;
  img_cfg.pitch = pitch;
  img_cfg.fmt = fmt;
  img_cfg.is_const = is_const;
  return HAL_IMPL_NAMESPACE::create_img(*inner, img_cfg);
}
Image Context::create_sampled_img(
  const std::string& label,
  MemoryAccess host_access,
  MemoryAccess dev_access,
  size_t nrow,
  size_t ncol,
  PixelFormat fmt
) const {
  return create_img(label, host_access, dev_access, nrow, ncol, fmt, true);
}
Image Context::create_storage_img(
  const std::string& label,
  MemoryAccess host_access,
  MemoryAccess dev_access,
  size_t nrow,
  size_t ncol,
  size_t pitch,
  PixelFormat fmt
) const {
  return create_img(label, host_access, dev_access, nrow, ncol, fmt, false);
}

Transaction Context::create_transact(
  const std::vector<Command>& cmds
) const {
  return HAL_IMPL_NAMESPACE::create_transact(*inner, cmds.data(), cmds.size());
}

CommandDrain Context::create_cmd_drain() const {
  return HAL_IMPL_NAMESPACE::create_cmd_drain(*inner);
}



Buffer::Buffer(const Context& ctxt, const BufferConfig& cfg) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Buffer>(
    create_buf(*ctxt.inner, cfg))) {}
Buffer::Buffer(HAL_IMPL_NAMESPACE::Buffer&& inner) :
    inner(std::make_unique<HAL_IMPL_NAMESPACE::Buffer>(inner)) {}
Buffer::~Buffer() { HAL_IMPL_NAMESPACE::destroy_buf(*inner); }



Image::Image(const Context& ctxt, const ImageConfig& cfg) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Image>(
    create_img(*ctxt.inner, cfg))) {}
Image::Image(HAL_IMPL_NAMESPACE::Image&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Image>(inner)) {}
Image::~Image() { HAL_IMPL_NAMESPACE::destroy_img(*inner); }



Task::Task(const Context& ctxt, const ComputeTaskConfig& cfg) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Task>(
    create_comp_task(*ctxt.inner, cfg))) {}
Task::Task(HAL_IMPL_NAMESPACE::Task&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Task>(std::move(inner))) {}
Task::~Task() { destroy_task(*inner); }

ResourcePool Task::create_rsc_pool() const {
  return HAL_IMPL_NAMESPACE::create_rsc_pool(*inner->ctxt, *inner);
}
Framebuffer Task::create_framebuf(const ImageView& img_view) const {
  return HAL_IMPL_NAMESPACE::create_framebuf(*inner->ctxt, *inner, img_view);
}



Framebuffer::Framebuffer(
  const Context& ctxt,
  const Task& task,
  const ImageView& img_view
) : Framebuffer(create_framebuf(*ctxt.inner, *task.inner, img_view)) {}
Framebuffer::Framebuffer(HAL_IMPL_NAMESPACE::Framebuffer&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Framebuffer>(std::move(inner))) {}
Framebuffer::~Framebuffer() { destroy_framebuf(*inner); }



ResourcePool::ResourcePool(const Context& ctxt, const Task& task) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::ResourcePool>(
    create_rsc_pool(*ctxt.inner, *task.inner))) {}
ResourcePool::ResourcePool(HAL_IMPL_NAMESPACE::ResourcePool&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::ResourcePool>(inner)) {}
ResourcePool::~ResourcePool() { destroy_rsc_pool(*inner); }



Transaction::Transaction(
  const Context& ctxt,
  const Command* cmds,
  size_t ncmd
) : inner(std::make_unique<HAL_IMPL_NAMESPACE::Transaction>(
    create_transact(*ctxt.inner, cmds, ncmd))) {}
Transaction::Transaction(HAL_IMPL_NAMESPACE::Transaction&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Transaction>(inner)) {}
Transaction::Transaction(
  const Context& ctxt,
  const std::vector<Command>& cmds
) : Transaction(ctxt, cmds.data(), cmds.size()) {}
Transaction::~Transaction() { destroy_transact(*inner); }



CommandDrain::CommandDrain(const Context& ctxt) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::CommandDrain>(
    create_cmd_drain(*ctxt.inner))) {}
CommandDrain::CommandDrain(HAL_IMPL_NAMESPACE::CommandDrain&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::CommandDrain>(inner)) {}
CommandDrain::~CommandDrain() { destroy_cmd_drain(*inner); }

} // namespace scoped
} // namespace HAL_IMPL_NAMESPACE
} // namespace liong
