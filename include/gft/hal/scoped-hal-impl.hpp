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
  L_OBJECT_TYPE_SWAPCHAIN,
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
  L_CASE_DESTROY_OBJ(L_OBJECT_TYPE_SWAPCHAIN, destroy_swapchain, Swapchain);
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
  case L_OBJECT_TYPE_SWAPCHAIN: return "swapchain";
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
    L_ASSERT(gc_stack.size() > 1);
    L_ASSERT(gc_stack.back().label == label,
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
  HAL_IMPL_NAMESPACE::ty* reg_gc_frame_obj(HAL_IMPL_NAMESPACE::ty&& x) { \
    void* obj = new HAL_IMPL_NAMESPACE::ty( \
      std::forward<HAL_IMPL_NAMESPACE::ty>(x)); \
    GcEntry entry {}; \
    entry.obj_ty = ty_enum; \
    entry.obj = obj; \
    OBJ_POOL.gc_stack.back().entries.emplace_back(std::move(entry)); \
    return (HAL_IMPL_NAMESPACE::ty*)obj; \
  } \
  HAL_IMPL_NAMESPACE::ty* reg_raii_obj(HAL_IMPL_NAMESPACE::ty&& x) { \
    void* obj = new HAL_IMPL_NAMESPACE::ty( \
      std::forward<HAL_IMPL_NAMESPACE::ty>(x)); \
    std::pair<void*, ObjectType> extern_obj {}; \
    extern_obj.first = obj; \
    extern_obj.second = ty_enum; \
    OBJ_POOL.extern_objs.insert(std::move(extern_obj)); \
    return (HAL_IMPL_NAMESPACE::ty*)obj; \
  }

L_DEF_REG_GC(Context, L_OBJECT_TYPE_CONTEXT);
L_DEF_REG_GC(Buffer, L_OBJECT_TYPE_BUFFER);
L_DEF_REG_GC(Image, L_OBJECT_TYPE_IMAGE);
L_DEF_REG_GC(DepthImage, L_OBJECT_TYPE_DEPTH_IMAGE);
L_DEF_REG_GC(Swapchain, L_OBJECT_TYPE_SWAPCHAIN);
L_DEF_REG_GC(RenderPass, L_OBJECT_TYPE_RENDER_PASS);
L_DEF_REG_GC(Task, L_OBJECT_TYPE_TASK);
L_DEF_REG_GC(Invocation, L_OBJECT_TYPE_INVOCATION);
L_DEF_REG_GC(Transaction, L_OBJECT_TYPE_TRANSACTION);

#undef L_DEF_REG_GC

void destroy_raii_obj(void* obj) {
  if (obj == nullptr) {
    log::warn("attempted to destroy an external null object");
    return;
  }
  auto it = OBJ_POOL.extern_objs.find(obj);
  if (it == OBJ_POOL.extern_objs.end()) {
    log::warn("attempted to release unregistered external scoped obj");
  } else {
    OBJ_POOL.extern_objs.erase(it);
  }
}

#define L_DEF_CTOR_DTOR(ty) \
  ty ty::borrow(const HAL_IMPL_NAMESPACE::ty& inner) { \
    ty out {}; \
    out.inner = (HAL_IMPL_NAMESPACE::ty*)&inner; \
    out.ownership = L_SCOPED_OBJECT_OWNERSHIP_BORROWED; \
    return out; \
  } \
  ty ty::own_by_raii(HAL_IMPL_NAMESPACE::ty&& inner) { \
    ty out {}; \
    out.inner = reg_raii_obj(std::forward<HAL_IMPL_NAMESPACE::ty>(inner)); \
    out.ownership = L_SCOPED_OBJECT_OWNERSHIP_OWNED_BY_RAII; \
    return out; \
  } \
  ty ty::own_by_gc_frame(HAL_IMPL_NAMESPACE::ty&& inner) { \
    ty out {}; \
    out.inner = reg_gc_frame_obj(std::forward<HAL_IMPL_NAMESPACE::ty>(inner)); \
    out.ownership = L_SCOPED_OBJECT_OWNERSHIP_OWNED_BY_GC_FRAME; \
    return out; \
  } \
  ty::~ty() { \
    if (inner != nullptr) { \
      if (ownership == L_SCOPED_OBJECT_OWNERSHIP_OWNED_BY_RAII) { \
        destroy_raii_obj(inner); \
      } \
      inner = nullptr; \
    } \
  }

#define L_BUILD_WITH_CFG(ty, create_fn) \
  gc ? \
    ty::own_by_gc_frame(create_fn(parent, inner)) : \
    ty::own_by_raii(create_fn(parent, inner));



L_DEF_CTOR_DTOR(Context);


L_DEF_CTOR_DTOR(Buffer);
BufferBuilder& BufferBuilder::streaming_with_aligned(
  const void* data,
  size_t elem_size,
  size_t elem_align,
  size_t nelem
) {
  if (elem_align == 0) {
    elem_align = 1;
  }
  size_t elem_aligned_size = util::align_up(elem_size, elem_align);
  L_ASSERT(inner.size == elem_aligned_size * nelem || inner.size == 0,
    "buffer streaming must cover the entire range");
  streaming_data = data;
  streaming_elem_size = elem_size;
  streaming_elem_size_aligned = elem_aligned_size;
  nstreaming_elem = nelem;
  return streaming().size(elem_aligned_size * nelem);
}
Buffer BufferBuilder::build(bool gc) {
  auto out = L_BUILD_WITH_CFG(Buffer, create_buf);
  if (streaming_data != nullptr && nstreaming_elem > 0) {
    MappedBuffer mapped = out.map_write();
    size_t offset_src = 0;
    size_t offset_dst = 0;
    for (size_t i = 0; i < nstreaming_elem; ++i) {
      std::memcpy(
        ((uint8_t*)mapped) + offset_dst,
        ((const uint8_t*)streaming_data) + offset_src,
        streaming_elem_size);
      offset_src += streaming_elem_size;
      offset_dst += streaming_elem_size_aligned;
    }
  }
  return std::move(out);
}



L_DEF_CTOR_DTOR(Image);
Image ImageBuilder::build(bool gc) {
  return L_BUILD_WITH_CFG(Image, create_img);
}



L_DEF_CTOR_DTOR(DepthImage);
DepthImage DepthImageBuilder::build(bool gc) {
  return L_BUILD_WITH_CFG(DepthImage, create_depth_img);
}



L_DEF_CTOR_DTOR(Swapchain);
Swapchain SwapchainBuilder::build(bool gc) {
  return L_BUILD_WITH_CFG(Swapchain, create_swapchain);
}
Invocation Swapchain::create_present_invoke(bool gc) const {
  return gc ?
    Invocation::own_by_gc_frame(HAL_IMPL_NAMESPACE::create_present_invoke(*inner)) :
    Invocation::own_by_raii(HAL_IMPL_NAMESPACE::create_present_invoke(*inner));
}



L_DEF_CTOR_DTOR(RenderPass);
RenderPass RenderPassBuilder::build(bool gc) {
  return L_BUILD_WITH_CFG(RenderPass, create_pass);
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



L_DEF_CTOR_DTOR(Task);
Task ComputeTaskBuilder::build(bool gc) {
  return L_BUILD_WITH_CFG(Task, create_comp_task);
}
Task GraphicsTaskBuilder::build(bool gc) {
  return L_BUILD_WITH_CFG(Task, create_graph_task);
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



L_DEF_CTOR_DTOR(Invocation);
Invocation TransferInvocationBuilder::build(bool gc) {
  return L_BUILD_WITH_CFG(Invocation, create_trans_invoke);
}
Invocation ComputeInvocationBuilder::build(bool gc) {
  return L_BUILD_WITH_CFG(Invocation, create_comp_invoke);
}
Invocation GraphicsInvocationBuilder::build(bool gc) {
  return L_BUILD_WITH_CFG(Invocation, create_graph_invoke);
}
Invocation RenderPassInvocationBuilder::build(bool gc) {
  return L_BUILD_WITH_CFG(Invocation, create_pass_invoke);
}
Invocation CompositeInvocationBuilder::build(bool gc) {
  return L_BUILD_WITH_CFG(Invocation, create_composite_invoke);
}
Transaction Invocation::submit(bool gc) {
  return gc ?
    Transaction::own_by_gc_frame(submit_invoke(*inner)) :
    Transaction::own_by_raii(submit_invoke(*inner));
}



L_DEF_CTOR_DTOR(Transaction);



#undef L_DEF_CTOR_DTOR

} // namespace scoped
} // namespace HAL_IMPL_NAMESPACE
} // namespace liong
