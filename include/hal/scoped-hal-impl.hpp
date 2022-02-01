#pragma once
#include "hal/scoped-hal.hpp"

#ifndef HAL_IMPL_NAMESPACE
static_assert(false, "please specify the implementation namespace (e.g. `vk`)");
#endif

namespace liong {
namespace HAL_IMPL_NAMESPACE {
namespace scoped {

Context::Context(const ContextConfig& cfg) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Context>(create_ctxt(cfg))) {}
Context::Context(HAL_IMPL_NAMESPACE::Context&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Context>(
    std::forward<HAL_IMPL_NAMESPACE::Context>(inner))) {}
Context::Context(const std::string& label, uint32_t dev_idx) :
  Context(ContextConfig { label, dev_idx }) {}
Context::~Context() {
  if (inner != nullptr) {
    HAL_IMPL_NAMESPACE::destroy_ctxt(*inner);
  }
}

ComputeTaskBuilder Context::build_comp_task(const std::string& label) const {
  return ComputeTaskBuilder(*this, label);
}
RenderPassBuilder Context::build_pass(const std::string& label) const {
  return RenderPassBuilder(*this, label);
}
BufferBuilder Context::build_buf(const std::string& label) const {
  return BufferBuilder(*this, label);
}
ImageBuilder Context::build_img(const std::string& label) const {
  return ImageBuilder(*this, label);
}
DepthImageBuilder Context::build_depth_img(const std::string& label) const {
  return DepthImageBuilder(*this, label);
}

void _copy_img_tile(
  void* dst,
  size_t dst_row_pitch,
  size_t dst_y_offset,
  size_t dst_local_offset, // Offset starting from base of each row.
  void* src,
  size_t src_row_pitch,
  size_t src_y_offset,
  size_t src_local_offset,
  size_t height,
  size_t row_size
) {
  uint8_t* dst_typed = (uint8_t*)dst;
  uint8_t* src_typed = (uint8_t*)src;

  for (size_t row = 0; row < height; ++row) {
    size_t dst_offset =
      (dst_y_offset + row) * dst_row_pitch + dst_local_offset;
    size_t src_offset =
      (src_y_offset + row) * src_row_pitch + src_local_offset;
    std::memcpy(
      (uint8_t*)dst_typed + dst_offset,
      (uint8_t*)src_typed + src_offset,
      row_size);
  }
}
MappedImage::MappedImage(const ImageView& view, MemoryAccess map_access) :
  mapped(nullptr),
  row_pitch(0),
  view(view),
  buf(nullptr),
  buf_size(0)
{
  map_img_mem(view, map_access, mapped, row_pitch);

  const auto& img_cfg = view.img->img_cfg;
  size_t fmt_size = img_cfg.fmt.get_fmt_size();
  size_t expected_pitch = fmt_size * img_cfg.width;

  bool need_stage_buf = false;
  if (view.width != img_cfg.width || view.height != img_cfg.height) {
    log::warn("only a portion of the image is mapped; staging buffer will be "
      "used to relayout data on the host side");
    need_stage_buf = true;
  } else if (row_pitch != expected_pitch) {
    log::warn("image allocation size is not aligned to required pitch (expect=",
      expected_pitch, ", actual=", row_pitch, "); staging buffer will be used "
      "to relayout data on the host side");
    need_stage_buf = true;
  }
  if (need_stage_buf) {
    buf_size = expected_pitch * img_cfg.height;
    buf = new uint8_t[buf_size];
  }

  // Copy the image on device to the staging buffer if the user wants to read
  // the content.
  if (buf != nullptr && (map_access & L_MEMORY_ACCESS_READ_BIT)) {
    size_t buf_row_pitch = expected_pitch;
    _copy_img_tile(buf, expected_pitch, 0, 0, mapped, row_pitch,
      view.y_offset, view.x_offset * fmt_size, view.height, expected_pitch);
  }
}
MappedImage::~MappedImage() {
  if (buf != nullptr) {
    const auto& img_cfg = view.img->img_cfg;
    size_t fmt_size = img_cfg.fmt.get_fmt_size();
    size_t buf_row_pitch = view.width * fmt_size;

    _copy_img_tile(mapped, row_pitch, view.y_offset,
      view.x_offset * fmt_size, buf, buf_row_pitch, 0, 0, view.height,
      buf_row_pitch);

    delete [] (uint8_t*)buf;
    buf = nullptr;
  }
  unmap_img_mem(view, mapped);
}

Transaction Context::create_transact(
  const std::string& label,
  const std::vector<Command>& cmds
) const {
  return HAL_IMPL_NAMESPACE::create_transact(label, *inner, cmds.data(),
    cmds.size());
}

CommandDrain Context::create_cmd_drain() const {
  return HAL_IMPL_NAMESPACE::create_cmd_drain(*inner);
}



Buffer::Buffer(HAL_IMPL_NAMESPACE::Buffer&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Buffer>(
    std::forward<HAL_IMPL_NAMESPACE::Buffer>(inner))),
  dont_destroy(false) {}
Buffer::~Buffer() {
  if (!dont_destroy && inner != nullptr) {
    HAL_IMPL_NAMESPACE::destroy_buf(*inner);
  }
}
Buffer BufferBuilder::build() {
  return create_buf(parent, inner);
}



Image::Image(HAL_IMPL_NAMESPACE::Image&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Image>(
    std::forward<HAL_IMPL_NAMESPACE::Image>(inner))),
  dont_destroy(false) {}
Image::~Image() {
  if (!dont_destroy && inner != nullptr) {
    HAL_IMPL_NAMESPACE::destroy_img(*inner);
  }
}
Image ImageBuilder::build() {
  return create_img(parent, inner);
}



DepthImage::DepthImage(HAL_IMPL_NAMESPACE::DepthImage&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::DepthImage>(
    std::forward<HAL_IMPL_NAMESPACE::DepthImage>(inner))) {}
DepthImage::~DepthImage() { HAL_IMPL_NAMESPACE::destroy_depth_img(*inner); }
DepthImage DepthImageBuilder::build() {
  return create_depth_img(parent, inner);
}



RenderPass::RenderPass(HAL_IMPL_NAMESPACE::RenderPass&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::RenderPass>(
    std::forward<HAL_IMPL_NAMESPACE::RenderPass>(inner))) {}
RenderPass::~RenderPass() { HAL_IMPL_NAMESPACE::destroy_pass(*inner); }
RenderPass RenderPassBuilder::build() {
  return create_pass(parent, inner);
}

GraphicsTaskBuilder RenderPass::build_graph_task(
  const std::string& label
) const {
  return GraphicsTaskBuilder(*this, label);
}
RenderPassInvocationBuilder RenderPass::build_pass_invoke(
  const std::string& label
) const {
  return RenderPassInvocationBuilder(*this, label);
}



Task::Task(HAL_IMPL_NAMESPACE::Task&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Task>(
    std::forward<HAL_IMPL_NAMESPACE::Task>(inner))) {}
Task::~Task() {
  if (inner != nullptr) {
    destroy_task(*inner);
  }
}
Task ComputeTaskBuilder::build() {
  return create_comp_task(parent, inner);
}
Task GraphicsTaskBuilder::build() {
  return create_graph_task(parent, inner);
}

ComputeInvocationBuilder Task::build_comp_invoke(
  const std::string& label
) const {
  return ComputeInvocationBuilder(*this, label);
}
GraphicsInvocationBuilder Task::build_graph_invoke(
  const std::string& label
) const {
  return GraphicsInvocationBuilder(*this, label);
}


Invocation::Invocation(HAL_IMPL_NAMESPACE::Invocation&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Invocation>(
    std::forward<HAL_IMPL_NAMESPACE::Invocation>(inner))) {}
Invocation::~Invocation() {
  if (inner != nullptr) {
    destroy_invoke(*inner);
  }
}
Invocation ComputeInvocationBuilder::build() {
  return create_comp_invoke(parent, inner);
}
Invocation GraphicsInvocationBuilder::build() {
  return create_graph_invoke(parent, inner);
}
Invocation RenderPassInvocationBuilder::build() {
  return create_pass_invoke(parent, inner);
}



Transaction::Transaction(
  const Context& ctxt,
  const std::string& label,
  const Command* cmds,
  size_t ncmd
) : inner(std::make_unique<HAL_IMPL_NAMESPACE::Transaction>(
    create_transact(label, *ctxt.inner, cmds, ncmd))) {}
Transaction::Transaction(HAL_IMPL_NAMESPACE::Transaction&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Transaction>(
    std::forward<HAL_IMPL_NAMESPACE::Transaction>(inner))) {}
Transaction::Transaction(
  const Context& ctxt,
  const std::string& label,
  const std::vector<Command>& cmds
) : Transaction(ctxt, label, cmds.data(), cmds.size()) {}
Transaction::~Transaction() {
  if (inner != nullptr) {
    destroy_transact(*inner);
  }
}



CommandDrain::CommandDrain(const Context& ctxt) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::CommandDrain>(
    create_cmd_drain(*ctxt.inner))) {}
CommandDrain::CommandDrain(HAL_IMPL_NAMESPACE::CommandDrain&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::CommandDrain>(
    std::forward<HAL_IMPL_NAMESPACE::CommandDrain>(inner))) {}
CommandDrain::~CommandDrain() {
  if (inner != nullptr) {
    destroy_cmd_drain(*inner);
  }
}

} // namespace scoped
} // namespace HAL_IMPL_NAMESPACE
} // namespace liong
