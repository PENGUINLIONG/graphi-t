#pragma once
#include "gft/hal/scoped-hal.hpp"

#ifndef HAL_IMPL_NAMESPACE
static_assert(false, "please specify the implementation namespace (e.g. `vk`)");
#endif

namespace liong {
namespace HAL_IMPL_NAMESPACE {
namespace scoped {

enum GcObjectType {
  L_GC_OBJECT_TYPE_CONTEXT,
  L_GC_OBJECT_TYPE_BUFFER,
  L_GC_OBJECT_TYPE_IMAGE,
  L_GC_OBJECT_TYPE_DEPTH_IMAGE,
  L_GC_OBJECT_TYPE_RENDER_PASS,
  L_GC_OBJECT_TYPE_TASK,
  L_GC_OBJECT_TYPE_INVOCATION,
  L_GC_OBJECT_TYPE_TRANSACTION,
};
struct GcEntry {
  GcObjectType obj_ty;
  void* obj;
};
struct GcFrame {
  bool is_extern;
  // Use lists to ensure stable address.
  std::vector<GcEntry> entries;

  GcFrame(bool is_extern = false) : is_extern(is_extern) {
    if (!is_extern) {
      log::debug("entered gc frame");
    }
  }
  GcFrame(const GcFrame&) = delete;
  GcFrame(GcFrame&&) = default;
  ~GcFrame() {
    for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
      auto& entry = *it;
      switch (entry.obj_ty) {
      case L_GC_OBJECT_TYPE_CONTEXT:
        destroy_ctxt(*(HAL_IMPL_NAMESPACE::Context*)entry.obj);
        if (!is_extern) { delete (HAL_IMPL_NAMESPACE::Context*)entry.obj; }
        break;
      case L_GC_OBJECT_TYPE_BUFFER:
        destroy_buf(*(HAL_IMPL_NAMESPACE::Buffer*)entry.obj);
        if (!is_extern) { delete (HAL_IMPL_NAMESPACE::Buffer*)entry.obj; }
        break;
      case L_GC_OBJECT_TYPE_IMAGE:
        destroy_img(*(HAL_IMPL_NAMESPACE::Image*)entry.obj);
        if (!is_extern) { delete (HAL_IMPL_NAMESPACE::Image*)entry.obj; }
        break;
      case L_GC_OBJECT_TYPE_DEPTH_IMAGE:
        destroy_depth_img(*(HAL_IMPL_NAMESPACE::DepthImage*)entry.obj);
        if (!is_extern) { delete (HAL_IMPL_NAMESPACE::DepthImage*)entry.obj; }
        break;
      case L_GC_OBJECT_TYPE_RENDER_PASS:
        destroy_pass(*(HAL_IMPL_NAMESPACE::RenderPass*)entry.obj);
        if (!is_extern) { delete (HAL_IMPL_NAMESPACE::RenderPass*)entry.obj; }
        break;
      case L_GC_OBJECT_TYPE_TASK:
        destroy_task(*(HAL_IMPL_NAMESPACE::Task*)entry.obj);
        if (!is_extern) { delete (HAL_IMPL_NAMESPACE::Task*)entry.obj; }
        break;
      case L_GC_OBJECT_TYPE_INVOCATION:
        destroy_invoke(*(HAL_IMPL_NAMESPACE::Invocation*)entry.obj);
        if (!is_extern) { delete (HAL_IMPL_NAMESPACE::Invocation*)entry.obj; }
        break;
      case L_GC_OBJECT_TYPE_TRANSACTION:
        destroy_transact(*(HAL_IMPL_NAMESPACE::Transaction*)entry.obj);
        if (!is_extern) { delete (HAL_IMPL_NAMESPACE::Transaction*)entry.obj; }
        break;
      default: unreachable();
      }
    }
    entries.clear();
    if (!is_extern) {
      log::debug("exited gc frame");
    }
  }

};
// External resources, things here won't be released by GC.

std::vector<GcFrame> _init_gc_stack() {
  std::vector<GcFrame> out {};
  out.reserve(5);
  out.emplace_back(); // Global GC frame.
  return out;
}
static std::vector<GcFrame> GC_STACK = _init_gc_stack();
static GcFrame GC_EXTERN(true);

void push_gc_frame() {
  GC_STACK.emplace_back(false);
}
void pop_gc_frame(bool flatten) {
  assert(GC_STACK.size() > 1);
  if (flatten) {
    for (auto entry : GC_STACK.back().entries) {
      GC_STACK[GC_STACK.size() - 1].entries.emplace_back(entry);
    }
    GC_STACK.back().entries.clear();
  }
  GC_STACK.pop_back();
}



#define L_DEF_REG_GC(ty, ty_enum) \
  HAL_IMPL_NAMESPACE::ty* reg_gc_rsc(HAL_IMPL_NAMESPACE::ty&& x) { \
    GcEntry entry {}; \
    entry.obj_ty = ty_enum; \
    entry.obj = new HAL_IMPL_NAMESPACE::ty( \
        std::forward<HAL_IMPL_NAMESPACE::ty>(x)); \
    GC_STACK.back().entries.emplace_back(std::move(entry)); \
    return (HAL_IMPL_NAMESPACE::ty*)GC_STACK.back().entries.back().obj; \
  } \
  HAL_IMPL_NAMESPACE::ty* reg_extern_rsc(HAL_IMPL_NAMESPACE::ty&& x) { \
    GcEntry entry {}; \
    entry.obj_ty = ty_enum; \
    entry.obj = new HAL_IMPL_NAMESPACE::ty( \
        std::forward<HAL_IMPL_NAMESPACE::ty>(x)); \
    GC_EXTERN.entries.emplace_back(std::move(entry)); \
    return (HAL_IMPL_NAMESPACE::ty*)GC_EXTERN.entries.back().obj; \
  }

L_DEF_REG_GC(Context, L_GC_OBJECT_TYPE_CONTEXT);
L_DEF_REG_GC(Buffer, L_GC_OBJECT_TYPE_BUFFER);
L_DEF_REG_GC(Image, L_GC_OBJECT_TYPE_IMAGE);
L_DEF_REG_GC(DepthImage, L_GC_OBJECT_TYPE_DEPTH_IMAGE);
L_DEF_REG_GC(RenderPass, L_GC_OBJECT_TYPE_RENDER_PASS);
L_DEF_REG_GC(Task, L_GC_OBJECT_TYPE_TASK);
L_DEF_REG_GC(Invocation, L_GC_OBJECT_TYPE_INVOCATION);
L_DEF_REG_GC(Transaction, L_GC_OBJECT_TYPE_TRANSACTION);

#undef L_DEF_REG_GC



Context::Context(HAL_IMPL_NAMESPACE::Context&& inner) :
  inner(reg_gc_rsc(std::forward<HAL_IMPL_NAMESPACE::Context>(inner))) {}
Context::Context(const ContextConfig& cfg) :
  Context(create_ctxt(cfg)) {}
Context::Context(const std::string& label, uint32_t dev_idx) :
  Context(ContextConfig { label, dev_idx }) {}



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
TransferInvocationBuilder Context::build_trans_invoke(
  const std::string& label
) const {
  return TransferInvocationBuilder(*this, label);
}
CompositeInvocationBuilder Context::build_composite_invoke(
  const std::string& label
) const {
  return CompositeInvocationBuilder(*this, label);
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
  size_t fmt_size = fmt::get_fmt_size(img_cfg.fmt);
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
    size_t fmt_size = fmt::get_fmt_size(img_cfg.fmt);
    size_t buf_row_pitch = view.width * fmt_size;

    _copy_img_tile(mapped, row_pitch, view.y_offset,
      view.x_offset * fmt_size, buf, buf_row_pitch, 0, 0, view.height,
      buf_row_pitch);

    delete [] (uint8_t*)buf;
    buf = nullptr;
  }
  unmap_img_mem(view, mapped);
}



Buffer::Buffer(HAL_IMPL_NAMESPACE::Buffer&& inner) :
  inner(reg_gc_rsc(std::forward<HAL_IMPL_NAMESPACE::Buffer>(inner))) {}
Buffer BufferBuilder::build() {
  return create_buf(parent, inner);
}



Image::Image(HAL_IMPL_NAMESPACE::Image&& inner) :
  inner(reg_gc_rsc(std::forward<HAL_IMPL_NAMESPACE::Image>(inner))) {}
Image ImageBuilder::build() {
  return create_img(parent, inner);
}



DepthImage::DepthImage(HAL_IMPL_NAMESPACE::DepthImage&& inner) :
  inner(reg_gc_rsc(std::forward<HAL_IMPL_NAMESPACE::DepthImage>(inner))) {}
DepthImage DepthImageBuilder::build() {
  return create_depth_img(parent, inner);
}



RenderPass::RenderPass(HAL_IMPL_NAMESPACE::RenderPass&& inner) :
  inner(reg_gc_rsc(std::forward<HAL_IMPL_NAMESPACE::RenderPass>(inner))) {}
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
  inner(reg_gc_rsc(std::forward<HAL_IMPL_NAMESPACE::Task>(inner))) {}
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
  inner(reg_gc_rsc(std::forward<HAL_IMPL_NAMESPACE::Invocation>(inner))) {}
Invocation TransferInvocationBuilder::build() {
  return create_trans_invoke(parent, inner);
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
Invocation CompositeInvocationBuilder::build() {
  return create_composite_invoke(parent, inner);
}
Transaction Invocation::submit() {
  return create_transact(*inner);
}



Transaction::Transaction(HAL_IMPL_NAMESPACE::Transaction&& inner) :
  inner(reg_gc_rsc(std::forward<HAL_IMPL_NAMESPACE::Transaction>(inner))) {}

} // namespace scoped
} // namespace HAL_IMPL_NAMESPACE
} // namespace liong
