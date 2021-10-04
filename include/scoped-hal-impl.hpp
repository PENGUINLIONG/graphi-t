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

RenderPass Context::create_pass(const Image& attm) const {
  return HAL_IMPL_NAMESPACE::create_pass(*inner, *attm.inner);
}

Task Context::create_comp_task(
  const std::string& label,
  const std::string& entry_point,
  const void* code,
  const size_t code_size,
  const DispatchSize& workgrp_size,
  const std::vector<ResourceType>& rsc_tys
) const {
  ComputeTaskConfig comp_task_cfg {};
  comp_task_cfg.label = label;
  comp_task_cfg.entry_name = entry_point.c_str();
  comp_task_cfg.code = code;
  comp_task_cfg.code_size = code_size;
  comp_task_cfg.rsc_tys = rsc_tys.data();
  comp_task_cfg.nrsc_ty = rsc_tys.size();
  comp_task_cfg.workgrp_size = workgrp_size;
  return HAL_IMPL_NAMESPACE::create_comp_task(*inner, comp_task_cfg);
}

Buffer Context::create_buf(
  const std::string& label,
  BufferUsage usage,
  size_t size,
  size_t align
) const {
  MemoryAccess host_access = 0;
  MemoryAccess dev_access = 0;
  if (usage & L_BUFFER_USAGE_STAGING_BIT) {
    host_access |= L_MEMORY_ACCESS_READ_WRITE;
    dev_access |= L_MEMORY_ACCESS_READ_WRITE;
  }
  if (usage & L_BUFFER_USAGE_UNIFORM_BIT) {
    host_access |= L_MEMORY_ACCESS_WRITE_BIT;
    dev_access |= L_MEMORY_ACCESS_READ_BIT;
  }
  if (usage & L_BUFFER_USAGE_STORAGE_BIT) {
    host_access |= L_MEMORY_ACCESS_READ_WRITE;
    dev_access |= L_MEMORY_ACCESS_READ_WRITE;
  }
  if (usage & L_BUFFER_USAGE_VERTEX_BIT) {
    host_access |= L_MEMORY_ACCESS_WRITE_BIT;
    dev_access |= L_MEMORY_ACCESS_READ_BIT;
  }
  if (usage & L_BUFFER_USAGE_INDEX_BIT) {
    host_access |= L_MEMORY_ACCESS_WRITE_BIT;
    dev_access |= L_MEMORY_ACCESS_READ_BIT;
  }

  BufferConfig buf_cfg {};
  buf_cfg.label = label;
  buf_cfg.size = size;
  buf_cfg.align = align;
  buf_cfg.host_access = host_access;
  buf_cfg.dev_access = dev_access;
  buf_cfg.usage = usage;
  return HAL_IMPL_NAMESPACE::create_buf(*inner, buf_cfg);
}
Buffer Context::create_staging_buf(
  const std::string& label,
  size_t size,
  size_t align
) const {
  return create_buf(label, L_BUFFER_USAGE_STAGING_BIT, size, align);
}
Buffer Context::create_uniform_buf(
  const std::string& label,
  size_t size,
  size_t align
) const {
  return create_buf(label, L_BUFFER_USAGE_UNIFORM_BIT, size, align);
}
Buffer Context::create_storage_buf(
  const std::string& label,
  size_t size,
  size_t align
) const {
  return create_buf(label, L_BUFFER_USAGE_STORAGE_BIT, size, align);
}
Buffer Context::create_vert_buf(
  const std::string& label,
  size_t size,
  size_t align
) const {
  return create_buf(label, L_BUFFER_USAGE_VERTEX_BIT, size, align);
}
Buffer Context::create_idx_buf(
  const std::string& label,
  size_t size,
  size_t align
) const {
  return create_buf(label, L_BUFFER_USAGE_INDEX_BIT, size, align);
}

Image Context::create_img(
  const std::string& label,
  ImageUsage usage,
  size_t nrow,
  size_t ncol,
  PixelFormat fmt
) const {
  MemoryAccess host_access = L_MEMORY_ACCESS_NONE;
  MemoryAccess dev_access = L_MEMORY_ACCESS_NONE;
  if (usage & L_IMAGE_USAGE_STAGING_BIT) {
    host_access |= L_MEMORY_ACCESS_READ_WRITE;
    dev_access |= L_MEMORY_ACCESS_READ_BIT; 
  }
  if (usage & L_IMAGE_USAGE_SAMPLED_BIT) {
    dev_access |= L_MEMORY_ACCESS_READ_BIT;
  }
  if (usage & L_IMAGE_USAGE_STORAGE_BIT) {
    dev_access |= L_MEMORY_ACCESS_READ_WRITE;
  }
  if (usage & L_IMAGE_USAGE_ATTACHMENT_BIT) {
    dev_access |= L_MEMORY_ACCESS_WRITE_BIT;
  }

  ImageConfig img_cfg {};
  img_cfg.label = label;
  img_cfg.host_access = host_access;
  img_cfg.dev_access = dev_access;
  img_cfg.nrow = nrow;
  img_cfg.ncol = ncol;
  img_cfg.fmt = fmt;
  img_cfg.usage = usage;
  return HAL_IMPL_NAMESPACE::create_img(*inner, img_cfg);
}
Image Context::create_staging_img(
  const std::string& label,
  size_t nrow,
  size_t ncol,
  PixelFormat fmt
) const {
  return create_img(label, L_IMAGE_USAGE_STAGING_BIT, nrow, ncol, fmt);
}
Image Context::create_sampled_img(
  const std::string& label,
  size_t nrow,
  size_t ncol,
  PixelFormat fmt
) const {
  return create_img(label, L_IMAGE_USAGE_SAMPLED_BIT, nrow, ncol, fmt);
}
Image Context::create_storage_img(
  const std::string& label,
  size_t nrow,
  size_t ncol,
  PixelFormat fmt
) const {
  return create_img(label, L_IMAGE_USAGE_STORAGE_BIT, nrow, ncol, fmt);
}
Image Context::create_attm_img(
  const std::string& label,
  size_t nrow,
  size_t ncol,
  PixelFormat fmt
) const {
  return create_img(label, L_IMAGE_USAGE_ATTACHMENT_BIT, nrow, ncol, fmt);
}

void _copy_img_tile(
  void* dst,
  size_t dst_row_pitch,
  size_t dst_row_offset,
  size_t dst_local_offset, // Offset starting from base of each row.
  void* src,
  size_t src_row_pitch,
  size_t src_row_offset,
  size_t src_local_offset,
  size_t nrow,
  size_t row_size
) {
  uint8_t* dst_typed = (uint8_t*)dst;
  uint8_t* src_typed = (uint8_t*)src;

  for (size_t row = 0; row < nrow; ++row) {
    size_t dst_offset =
      (dst_row_offset + row) * dst_row_pitch + dst_local_offset;
    size_t src_offset =
      (src_row_offset + row) * src_row_pitch + src_local_offset;
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
  size_t expected_pitch = fmt_size * img_cfg.ncol;

  bool need_stage_buf = false;
  if (view.ncol != img_cfg.ncol || view.nrow != img_cfg.nrow) {
    liong::log::warn("only a portion of the image is mapped; staging buffer "
      "will be used to relayout data on the host side");
    need_stage_buf = true;
  } else if (row_pitch != expected_pitch) {
    liong::log::warn("image allocation size is not aligned to required "
      "pitch (expect=", expected_pitch, ", actual=", row_pitch, "); staging "
      "buffer will be used to relayout data on the host side");
    need_stage_buf = true;
  }
  if (need_stage_buf) {
    buf_size = expected_pitch * img_cfg.nrow;
    buf = new uint8_t[buf_size];
  }

  // Copy the image on device to the staging buffer if the user wants to read
  // the content.
  if (buf != nullptr && (map_access & L_MEMORY_ACCESS_READ_BIT)) {
    size_t buf_row_pitch = expected_pitch;
    _copy_img_tile(buf, expected_pitch, 0, 0, mapped, row_pitch,
      view.row_offset, view.col_offset * fmt_size, view.nrow, expected_pitch);
  }
}
MappedImage::~MappedImage() {
  if (buf != nullptr) {
    const auto& img_cfg = view.img->img_cfg;
    size_t fmt_size = img_cfg.fmt.get_fmt_size();
    size_t buf_row_pitch = view.ncol * fmt_size;

    _copy_img_tile(mapped, row_pitch, view.row_offset,
      view.col_offset * fmt_size, buf, buf_row_pitch, 0, 0, view.nrow,
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
  if (!dont_destroy) {
    HAL_IMPL_NAMESPACE::destroy_buf(*inner);
  }
}



Image::Image(const Context& ctxt, const ImageConfig& cfg) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Image>(
    create_img(*ctxt.inner, cfg))),
  dont_destroy(false) {}
Image::Image(HAL_IMPL_NAMESPACE::Image&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Image>(inner)),
  dont_destroy(false) {}
Image::~Image() {
  if (!dont_destroy) {
    HAL_IMPL_NAMESPACE::destroy_img(*inner);
  }
}



Task::Task(const Context& ctxt, const ComputeTaskConfig& cfg) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Task>(
    create_comp_task(*ctxt.inner, cfg))) {}
Task::Task(HAL_IMPL_NAMESPACE::Task&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Task>(std::move(inner))) {}
Task::~Task() { destroy_task(*inner); }

ResourcePool Task::create_rsc_pool() const {
  return HAL_IMPL_NAMESPACE::create_rsc_pool(*inner->ctxt, *inner);
}



RenderPass::RenderPass(
  const Context& ctxt,
  const Image& attm
) : inner(std::make_unique<HAL_IMPL_NAMESPACE::RenderPass>(
  create_pass(*ctxt.inner, *attm.inner))) {}
RenderPass::RenderPass(HAL_IMPL_NAMESPACE::RenderPass&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::RenderPass>(inner)) {}
RenderPass::~RenderPass() { destroy_pass(*inner); }

Task RenderPass::create_graph_task(
  const std::string& label,
  const std::string& vert_entry_point,
  const void* vert_code,
  const size_t vert_code_size,
  const std::string& frag_entry_point,
  const void* frag_code,
  const size_t frag_code_size,
  const VertexInput* vert_inputs,
  size_t nvert_input,
  Topology topo,
  const std::vector<ResourceType>& rsc_tys
) const {
  GraphicsTaskConfig graph_task_cfg {};
  graph_task_cfg.label = label;
  graph_task_cfg.vert_entry_name = vert_entry_point.c_str();
  graph_task_cfg.vert_code = vert_code;
  graph_task_cfg.vert_code_size = vert_code_size;
  graph_task_cfg.frag_entry_name = frag_entry_point.c_str();
  graph_task_cfg.frag_code = frag_code;
  graph_task_cfg.frag_code_size = frag_code_size;
  graph_task_cfg.vert_inputs = vert_inputs;
  graph_task_cfg.nvert_input = nvert_input;
  graph_task_cfg.topo = topo;
  graph_task_cfg.rsc_tys = rsc_tys.data();
  graph_task_cfg.nrsc_ty = rsc_tys.size();
  return HAL_IMPL_NAMESPACE::create_graph_task(*inner, graph_task_cfg);
}



ResourcePool::ResourcePool(const Context& ctxt, const Task& task) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::ResourcePool>(
    create_rsc_pool(*ctxt.inner, *task.inner))) {}
ResourcePool::ResourcePool(HAL_IMPL_NAMESPACE::ResourcePool&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::ResourcePool>(inner)) {}
ResourcePool::~ResourcePool() { destroy_rsc_pool(*inner); }



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
Transaction::~Transaction() { destroy_transact(*inner); }



Timestamp::Timestamp(
  const Context& ctxt
) : inner(std::make_unique<HAL_IMPL_NAMESPACE::Timestamp>(
  create_timestamp(*ctxt.inner))) {}
Timestamp::Timestamp(HAL_IMPL_NAMESPACE::Timestamp&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::Timestamp>(inner)) {}
Timestamp::~Timestamp() { if (inner) { destroy_timestamp(*inner); } }



CommandDrain::CommandDrain(const Context& ctxt) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::CommandDrain>(
    create_cmd_drain(*ctxt.inner))) {}
CommandDrain::CommandDrain(HAL_IMPL_NAMESPACE::CommandDrain&& inner) :
  inner(std::make_unique<HAL_IMPL_NAMESPACE::CommandDrain>(inner)) {}
CommandDrain::~CommandDrain() { destroy_cmd_drain(*inner); }

} // namespace scoped
} // namespace HAL_IMPL_NAMESPACE
} // namespace liong
