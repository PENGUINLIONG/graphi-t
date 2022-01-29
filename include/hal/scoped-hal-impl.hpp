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
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Context>(inner)) {}
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

Image Context::create_img(
  const std::string& label,
  ImageUsage usage,
  size_t height,
  size_t width,
  PixelFormat fmt
) const {
  MemoryAccess host_access = 0;
  MemoryAccess dev_access = 0;
  if (usage & L_IMAGE_USAGE_STAGING_BIT) {
    host_access |= L_MEMORY_ACCESS_READ_BIT | L_MEMORY_ACCESS_WRITE_BIT;
    dev_access |= L_MEMORY_ACCESS_READ_BIT; 
  }
  if (usage & L_IMAGE_USAGE_SAMPLED_BIT) {
    dev_access |= L_MEMORY_ACCESS_READ_BIT;
  }
  if (usage & L_IMAGE_USAGE_STORAGE_BIT) {
    dev_access |= L_MEMORY_ACCESS_READ_BIT | L_MEMORY_ACCESS_WRITE_BIT;
  }
  if (usage & L_IMAGE_USAGE_ATTACHMENT_BIT) {
    dev_access |= L_MEMORY_ACCESS_WRITE_BIT;
  }

  ImageConfig img_cfg {};
  img_cfg.label = label;
  img_cfg.host_access = host_access;
  img_cfg.dev_access = dev_access;
  img_cfg.height = height;
  img_cfg.width = width;
  img_cfg.fmt = fmt;
  img_cfg.usage = usage;
  return HAL_IMPL_NAMESPACE::create_img(*inner, img_cfg);
}
Image Context::create_staging_img(
  const std::string& label,
  size_t height,
  size_t width,
  PixelFormat fmt
) const {
  return create_img(label, L_IMAGE_USAGE_STAGING_BIT, height, width, fmt);
}
Image Context::create_sampled_img(
  const std::string& label,
  size_t height,
  size_t width,
  PixelFormat fmt
) const {
  return create_img(label, L_IMAGE_USAGE_SAMPLED_BIT, height, width, fmt);
}
Image Context::create_storage_img(
  const std::string& label,
  size_t height,
  size_t width,
  PixelFormat fmt
) const {
  return create_img(label, L_IMAGE_USAGE_STORAGE_BIT, height, width, fmt);
}
Image Context::create_attm_img(
  const std::string& label,
  size_t height,
  size_t width,
  PixelFormat fmt
) const {
  return create_img(label, L_IMAGE_USAGE_ATTACHMENT_BIT, height, width, fmt);
}

DepthImage Context::create_depth_img(
    const std::string& label,
    DepthImageUsage usage,
    uint32_t width,
    uint32_t height,
    DepthFormat depth_fmt
) const {
  DepthImageConfig depth_img_cfg {};
  depth_img_cfg.label = label;
  depth_img_cfg.width = width;
  depth_img_cfg.height = height;
  depth_img_cfg.fmt = depth_fmt;
  depth_img_cfg.usage = usage;
  return HAL_IMPL_NAMESPACE::create_depth_img(*inner, depth_img_cfg);
}
DepthImage Context::create_depth_img(
    const std::string& label,
    uint32_t width,
    uint32_t height,
    DepthFormat depth_fmt
) const {
  DepthImageUsage usage =
    L_DEPTH_IMAGE_USAGE_ATTACHMENT_BIT | L_DEPTH_IMAGE_USAGE_SAMPLED_BIT;
  return create_depth_img(label, usage, width, height, depth_fmt);
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
  view(view)
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

Timestamp Context::create_timestamp() const {
  return HAL_IMPL_NAMESPACE::create_timestamp(*inner);
}



Buffer::Buffer(const Context& ctxt, const BufferConfig& cfg) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Buffer>(
    create_buf(*ctxt.inner, cfg))),
  dont_destroy(false) {}
Buffer::Buffer(HAL_IMPL_NAMESPACE::Buffer&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Buffer>(inner)),
  dont_destroy(false) {}
Buffer::~Buffer() {
  if (!dont_destroy && inner != nullptr) {
    HAL_IMPL_NAMESPACE::destroy_buf(*inner);
  }
}
Buffer BufferBuilder::build() {
  return create_buf(parent, inner);
}



Image::Image(const Context& ctxt, const ImageConfig& cfg) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Image>(
    create_img(*ctxt.inner, cfg))),
  dont_destroy(false) {}
Image::Image(HAL_IMPL_NAMESPACE::Image&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Image>(inner)),
  dont_destroy(false) {}
Image::~Image() {
  if (!dont_destroy && inner != nullptr) {
    HAL_IMPL_NAMESPACE::destroy_img(*inner);
  }
}



DepthImage::DepthImage(const Context& ctxt, const DepthImageConfig& cfg) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::DepthImage>(
    create_depth_img(*ctxt.inner, cfg))) {}
DepthImage::DepthImage(HAL_IMPL_NAMESPACE::DepthImage&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::DepthImage>(inner)) {}
DepthImage::~DepthImage() { HAL_IMPL_NAMESPACE::destroy_depth_img(*inner); }



RenderPass::RenderPass(const Context& ctxt, const RenderPassConfig& cfg) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::RenderPass>(create_pass(ctxt, cfg))) {}
RenderPass::RenderPass(HAL_IMPL_NAMESPACE::RenderPass&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::RenderPass>(inner)) {}
RenderPass::~RenderPass() { HAL_IMPL_NAMESPACE::destroy_pass(*inner); }
RenderPass RenderPassBuilder::build() {
  return create_pass(parent, inner);
}

GraphicsTaskBuilder RenderPass::build_graph_task(
  const std::string& label
) const {
  return GraphicsTaskBuilder(*this, label);
}



Task::Task(const Context& ctxt, const ComputeTaskConfig& cfg) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Task>(
    create_comp_task(*ctxt.inner, cfg))) {}
Task::Task(HAL_IMPL_NAMESPACE::Task&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Task>(std::move(inner))) {}
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



ResourcePool Task::create_rsc_pool() const {
  return HAL_IMPL_NAMESPACE::create_rsc_pool(*inner);
}

ResourcePool::ResourcePool(const Context& ctxt, const Task& task) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::ResourcePool>(
    create_rsc_pool(*task.inner))) {}
ResourcePool::ResourcePool(HAL_IMPL_NAMESPACE::ResourcePool&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::ResourcePool>(inner)) {}
ResourcePool::~ResourcePool() {
  if (inner != nullptr) {
    destroy_rsc_pool(*inner);
  }
}



Transaction::Transaction(
  const Context& ctxt,
  const std::string& label,
  const Command* cmds,
  size_t ncmd
) : inner(std::make_unique<HAL_IMPL_NAMESPACE::Transaction>(
    create_transact(label, *ctxt.inner, cmds, ncmd))) {}
Transaction::Transaction(HAL_IMPL_NAMESPACE::Transaction&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Transaction>(inner)) {}
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



Timestamp::Timestamp(
  const Context& ctxt
) : inner(std::make_unique<HAL_IMPL_NAMESPACE::Timestamp>(
  create_timestamp(*ctxt.inner))) {}
Timestamp::Timestamp(HAL_IMPL_NAMESPACE::Timestamp&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Timestamp>(inner)) {}
Timestamp::~Timestamp() {
  if (inner != nullptr) {
    destroy_timestamp(*inner);
  }
}



CommandDrain::CommandDrain(const Context& ctxt) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::CommandDrain>(
    create_cmd_drain(*ctxt.inner))) {}
CommandDrain::CommandDrain(HAL_IMPL_NAMESPACE::CommandDrain&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::CommandDrain>(inner)) {}
CommandDrain::~CommandDrain() {
  if (inner != nullptr) {
    destroy_cmd_drain(*inner);
  }
}

} // namespace scoped
} // namespace HAL_IMPL_NAMESPACE
} // namespace liong
