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
  ty::ty(HAL_IMPL_NAMESPACE::ty&& inner, bool gc) : \
    inner(reg_obj(std::forward<HAL_IMPL_NAMESPACE::ty>(inner), gc)), \
    gc(gc) {} \
  ty::~ty() { \
    if (inner != nullptr) { \
      if (!gc) { destroy_extern_obj(inner); } \
      inner = nullptr; \
    } \
  }



L_DEF_CTOR_DTOR(Context);


L_DEF_CTOR_DTOR(Buffer);
BufferBuilder& BufferBuilder::streaming_with(const void* data, size_t size) {
  assert(inner.size == size || inner.size == 0,
    "buffer streaming must cover the entire range");
  streaming_data = data;
  streaming_data_size = size;
  return streaming().size(size);
}
Buffer BufferBuilder::build(bool gc) {
  auto out = Buffer(create_buf(parent, inner), gc);
  if (streaming_data != nullptr && streaming_data_size > 0) {
    out.map_write().write(streaming_data, streaming_data_size);
  }
  return std::move(out);
}



L_DEF_CTOR_DTOR(Image);
Image ImageBuilder::build(bool gc) {
  return Image(create_img(parent, inner), gc);
}



L_DEF_CTOR_DTOR(DepthImage);
DepthImage DepthImageBuilder::build(bool gc) {
  return DepthImage(create_depth_img(parent, inner), gc);
}



L_DEF_CTOR_DTOR(RenderPass);
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



L_DEF_CTOR_DTOR(Task);
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



L_DEF_CTOR_DTOR(Invocation);
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
  return submit_invoke(*inner);
}



L_DEF_CTOR_DTOR(Transaction);



} // namespace scoped
} // namespace HAL_IMPL_NAMESPACE
} // namespace liong
