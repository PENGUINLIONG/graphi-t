#pragma once
#include "gft/hal/scoped-hal.hpp"

#ifndef HAL_IMPL_NAMESPACE
static_assert(false, "please specify the implementation namespace (e.g. `vk`)");
#endif

namespace liong {
namespace HAL_IMPL_NAMESPACE {
namespace scoped {

enum ObjectType {
  L_OBJECT_TYPE_CONTEXT,
  L_OBJECT_TYPE_BUFFER,
  L_OBJECT_TYPE_IMAGE,
  L_OBJECT_TYPE_DEPTH_IMAGE,
  L_OBJECT_TYPE_RENDER_PASS,
  L_OBJECT_TYPE_TASK,
  L_OBJECT_TYPE_INVOCATION,
  L_OBJECT_TYPE_TRANSACTION,
};
void _destroy_obj(ObjectType obj_ty, void* obj) {
  switch (obj_ty) {

#define L_CASE_DESTROY_OBJ(ty_enum, dtor, ty) \
  case ty_enum: \
    dtor(*(HAL_IMPL_NAMESPACE::ty*)obj); \
    delete (HAL_IMPL_NAMESPACE::ty*)obj; \
    break;

  L_CASE_DESTROY_OBJ(L_OBJECT_TYPE_CONTEXT, destroy_ctxt, Context);
  L_CASE_DESTROY_OBJ(L_OBJECT_TYPE_BUFFER, destroy_buf, Buffer);
  L_CASE_DESTROY_OBJ(L_OBJECT_TYPE_IMAGE, destroy_img, Image);
  L_CASE_DESTROY_OBJ(L_OBJECT_TYPE_DEPTH_IMAGE, destroy_depth_img, DepthImage);
  L_CASE_DESTROY_OBJ(L_OBJECT_TYPE_RENDER_PASS, destroy_pass, RenderPass);
  L_CASE_DESTROY_OBJ(L_OBJECT_TYPE_TASK, destroy_task, Task);
  L_CASE_DESTROY_OBJ(L_OBJECT_TYPE_INVOCATION, destroy_invoke, Invocation);
  L_CASE_DESTROY_OBJ(L_OBJECT_TYPE_TRANSACTION, destroy_transact, Transaction);

#undef L_CASE_DESTROY_OBJ

  default: unreachable();
  }
}
const char* _obj_ty2str(ObjectType obj_ty) {
  switch (obj_ty) {
  case L_OBJECT_TYPE_CONTEXT: return "context";
  case L_OBJECT_TYPE_BUFFER: return "buffer";
  case L_OBJECT_TYPE_IMAGE: return "image";
  case L_OBJECT_TYPE_DEPTH_IMAGE: return "depth image";
  case L_OBJECT_TYPE_RENDER_PASS: return "render pass";
  case L_OBJECT_TYPE_TASK: return "task";
  case L_OBJECT_TYPE_INVOCATION: return "invocation";
  case L_OBJECT_TYPE_TRANSACTION: return "transaction";
  default: unreachable();
  }
}
struct GcEntry {
  ObjectType obj_ty;
  void* obj;
};
struct GcFrame {
  std::string label;
  // Use lists to ensure stable address.
  std::vector<GcEntry> entries;

  GcFrame(const std::string& label) : label(label), entries() {
    log::debug("entered gc frame '", label, "'");
  }
  GcFrame(const GcFrame&) = delete;
  GcFrame(GcFrame&&) = default;
  ~GcFrame() {
    for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
      auto& entry = *it;
      _destroy_obj(it->obj_ty, it->obj);
    }
    entries.clear();
    log::debug("exited gc frame '", label, "'");
  }

};
struct ObjectPool {
  std::vector<GcFrame> gc_stack;
  std::map<void*, ObjectType> extern_objs;

  ObjectPool() : gc_stack(), extern_objs() {
    gc_stack.reserve(5);
    gc_stack.emplace_back("<global>");
  }
  ~ObjectPool() {
    while (gc_stack.size() > 1) {
      log::warn("process is terminating while the gc stack is fully popped; "
        "your object lifetime management should be reviewed");
      gc_stack.pop_back();
    }
    gc_stack.pop_back();
    for (auto it = extern_objs.begin(); it != extern_objs.end(); ++it) {
      _destroy_obj(it->second, it->first);
      log::warn("process is terminating while external ",
        _obj_ty2str(it->second), " is not yet destroyed; your object lifetime "
        "management should be reviewed");
    }
  }

  inline void push_frame(const std::string& label) {
    gc_stack.emplace_back(label);
  }
  inline void pop_frame(const std::string& label) {
    assert(gc_stack.size() > 1);
    assert(gc_stack.back().label == label,
      "gc frame label mismatched (expected=", gc_stack.back().label,
      "; actual=", label, ")");
    gc_stack.pop_back();
  }

} OBJ_POOL;

void push_gc_frame(const std::string& label) {
  OBJ_POOL.push_frame(label);
}
void pop_gc_frame(const std::string& label) {
  OBJ_POOL.pop_frame(label);
}



#define L_DEF_REG_GC(ty, ty_enum) \
  HAL_IMPL_NAMESPACE::ty* reg_obj(HAL_IMPL_NAMESPACE::ty&& x, bool gc) { \
    void* obj = new HAL_IMPL_NAMESPACE::ty( \
      std::forward<HAL_IMPL_NAMESPACE::ty>(x)); \
    if (gc) { \
      GcEntry entry {}; \
      entry.obj_ty = ty_enum; \
      entry.obj = obj; \
      OBJ_POOL.gc_stack.back().entries.emplace_back(std::move(entry)); \
    } else { \
      OBJ_POOL.extern_objs.insert( \
        std::make_pair<void*, ObjectType>( \
          std::move(obj), std::move(ty_enum))); \
    } \
    return (HAL_IMPL_NAMESPACE::ty*)obj; \
  }

L_DEF_REG_GC(Context, L_OBJECT_TYPE_CONTEXT);
L_DEF_REG_GC(Buffer, L_OBJECT_TYPE_BUFFER);
L_DEF_REG_GC(Image, L_OBJECT_TYPE_IMAGE);
L_DEF_REG_GC(DepthImage, L_OBJECT_TYPE_DEPTH_IMAGE);
L_DEF_REG_GC(RenderPass, L_OBJECT_TYPE_RENDER_PASS);
L_DEF_REG_GC(Task, L_OBJECT_TYPE_TASK);
L_DEF_REG_GC(Invocation, L_OBJECT_TYPE_INVOCATION);
L_DEF_REG_GC(Transaction, L_OBJECT_TYPE_TRANSACTION);

#undef L_DEF_REG_GC

void destroy_extern_obj(void* obj) {
  auto it = OBJ_POOL.extern_objs.find(obj);
  if (it == OBJ_POOL.extern_objs.end()) {
    log::warn("attempt to release unregistered external scoped obj");
  } else {
    OBJ_POOL.extern_objs.erase(it);
  }
}



Context::Context(HAL_IMPL_NAMESPACE::Context&& inner, bool gc) :
  inner(reg_obj(std::forward<HAL_IMPL_NAMESPACE::Context>(inner), gc)),
  gc(gc) {}
Context::~Context() {
  if (!gc && inner != nullptr) { destroy_extern_obj(inner); }
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



Buffer::Buffer(HAL_IMPL_NAMESPACE::Buffer&& inner, bool gc) :
  inner(reg_obj(std::forward<HAL_IMPL_NAMESPACE::Buffer>(inner), gc)),
  gc(gc) {}
Buffer::~Buffer() {
  if (!gc && inner != nullptr) { destroy_extern_obj(inner); }
}
Buffer BufferBuilder::build(bool gc) {
  return Buffer(create_buf(parent, inner), gc);
}



Image::Image(HAL_IMPL_NAMESPACE::Image&& inner, bool gc) :
  inner(reg_obj(std::forward<HAL_IMPL_NAMESPACE::Image>(inner), gc)),
  gc(gc) {}
Image::~Image() {
  if (!gc && inner != nullptr) { destroy_extern_obj(inner); }
}
Image ImageBuilder::build(bool gc) {
  return Image(create_img(parent, inner), gc);
}



DepthImage::DepthImage(HAL_IMPL_NAMESPACE::DepthImage&& inner, bool gc) :
  inner(reg_obj(std::forward<HAL_IMPL_NAMESPACE::DepthImage>(inner), gc)),
  gc(gc) {}
DepthImage::~DepthImage() {
  if (!gc && inner != nullptr) { destroy_extern_obj(inner); }
}
DepthImage DepthImageBuilder::build(bool gc) {
  return DepthImage(create_depth_img(parent, inner), gc);
}



RenderPass::RenderPass(HAL_IMPL_NAMESPACE::RenderPass&& inner, bool gc) :
  inner(reg_obj(std::forward<HAL_IMPL_NAMESPACE::RenderPass>(inner), gc)),
  gc(gc) {}
RenderPass::~RenderPass() {
  if (!gc && inner != nullptr) { destroy_extern_obj(inner); }
}
RenderPass RenderPassBuilder::build(bool gc) {
  return RenderPass(create_pass(parent, inner), gc);
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



Task::Task(HAL_IMPL_NAMESPACE::Task&& inner, bool gc) :
  inner(reg_obj(std::forward<HAL_IMPL_NAMESPACE::Task>(inner), gc)),
  gc(gc) {}
Task::~Task() {
  if (!gc && inner != nullptr) { destroy_extern_obj(inner); }
}
Task ComputeTaskBuilder::build(bool gc) {
  return Task(create_comp_task(parent, inner), gc);
}
Task GraphicsTaskBuilder::build(bool gc) {
  return Task(create_graph_task(parent, inner), gc);
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



Invocation::Invocation(HAL_IMPL_NAMESPACE::Invocation&& inner, bool gc) :
  inner(reg_obj(std::forward<HAL_IMPL_NAMESPACE::Invocation>(inner), gc)),
  gc(gc) {}
Invocation::~Invocation() { if (!gc) { destroy_extern_obj(inner); } }
Invocation TransferInvocationBuilder::build(bool gc) {
  return Invocation(create_trans_invoke(parent, inner), gc);
}
Invocation ComputeInvocationBuilder::build(bool gc) {
  return Invocation(create_comp_invoke(parent, inner), gc);
}
Invocation GraphicsInvocationBuilder::build(bool gc) {
  return Invocation(create_graph_invoke(parent, inner), gc);
}
Invocation RenderPassInvocationBuilder::build(bool gc) {
  return Invocation(create_pass_invoke(parent, inner), gc);
}
Invocation CompositeInvocationBuilder::build(bool gc) {
  return Invocation(create_composite_invoke(parent, inner), gc);
}
Transaction Invocation::submit() {
  return create_transact(*inner);
}



Transaction::Transaction(HAL_IMPL_NAMESPACE::Transaction&& inner, bool gc) :
  inner(reg_obj(std::forward<HAL_IMPL_NAMESPACE::Transaction>(inner), gc)),
  gc(gc) {}
Transaction::~Transaction() {
  if (!gc && inner != nullptr) { destroy_extern_obj(inner); }
}

} // namespace scoped
} // namespace HAL_IMPL_NAMESPACE
} // namespace liong
