#pragma once
#include "hal.hpp"

#ifndef HAL_IMPL_NAMESPACE
static_assert(false, "please specify the implementation namespace (e.g. `vk`)");
#endif

namespace liong {

namespace HAL_IMPL_NAMESPACE {

namespace scoped {

struct Context {
public:
  std::unique_ptr<HAL_IMPL_NAMESPACE::Context> inner;

  Context(const ContextConfig& cfg);
  inline ~Context() { destroy_ctxt(*inner); }

  inline operator HAL_IMPL_NAMESPACE::Context&() {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::Context&() const {
    return *inner;
  }

  inline const ContextConfig& cfg() const {
    return get_ctxt_cfg(*inner);
  }
};



struct MappedBuffer {
  void* mapped;
  BufferView view;

  inline MappedBuffer(const BufferView& view, MemoryAccess map_access) :
    mapped(nullptr),
    view(view)
  {
    map_mem(view, mapped, map_access);
  }
  inline ~MappedBuffer() {
    unmap_mem(view, mapped);
  }

  template<typename T, typename _ = std::enable_if_t<std::is_pointer_v<T>>>
  inline operator T() const {
    return (T)mapped;
  }
};
struct Buffer {
  std::unique_ptr<HAL_IMPL_NAMESPACE::Buffer> inner;

  Buffer(const Context& ctxt, const BufferConfig& cfg);
  inline ~Buffer() { destroy_buf(*inner); }

  
  inline operator HAL_IMPL_NAMESPACE::Buffer&() {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::Buffer&() const {
    return *inner;
  }

  inline const BufferConfig& cfg() const {
    return get_buf_cfg(*inner);
  }

  inline BufferView view(size_t offset, size_t size) const {
    return BufferView { &*inner, offset, size };
  }
  inline BufferView view() const {
    auto& buf_cfg = cfg();
    return view(0, buf_cfg.size);
  }

  inline MappedBuffer map(
    size_t offset,
    size_t size,
    MemoryAccess map_access
  ) const {
    return MappedBuffer(view(offset, size), map_access);
  }
  inline MappedBuffer map(MemoryAccess map_access) const {
    return MappedBuffer(view(), map_access);
  }
};



struct Image {
  std::unique_ptr<HAL_IMPL_NAMESPACE::Image> inner;

  Image(const Context& ctxt, const ImageConfig& cfg);
  inline ~Image() { destroy_img(*inner); }

  inline operator HAL_IMPL_NAMESPACE::Image& () {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::Image& () const {
    return *inner;
  }

  inline const ImageConfig& cfg() const {
    return get_img_cfg(*inner);
  }

  inline ImageView view(
    uint32_t nrow_offset,
    uint32_t ncol_offset,
    uint32_t nrow,
    uint32_t ncol
  ) const {
    return ImageView { &*inner, nrow_offset, ncol_offset, nrow, ncol };
  }
  inline ImageView view() const {
    auto& img_cfg = cfg();
    return view(0, 0, img_cfg.nrow, img_cfg.ncol);
  }
};



struct Task {
  std::unique_ptr<HAL_IMPL_NAMESPACE::Task> inner;

  Task(const Context& ctxt, const ComputeTaskConfig& cfg);
  inline ~Task() { destroy_task(*inner); }

  inline operator HAL_IMPL_NAMESPACE::Task& () {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::Task& () const {
    return *inner;
  }
};



struct ResourcePool {
  std::unique_ptr<HAL_IMPL_NAMESPACE::ResourcePool> inner;

  ResourcePool(const Context& ctxt, const Task& task);
  inline ~ResourcePool() { destroy_rsc_pool(*inner); }

  inline operator HAL_IMPL_NAMESPACE::ResourcePool& () {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::ResourcePool& () const {
    return *inner;
  }

  inline void bind(uint32_t idx, const BufferView& buf_view) {
    bind_pool_rsc(*inner, idx, buf_view);
  }
  inline void bind(uint32_t idx, const ImageView& img_view) {
    bind_pool_rsc(*inner, idx, img_view);
  }
};



struct Transaction {
  std::unique_ptr<HAL_IMPL_NAMESPACE::Transaction> inner;

  Transaction(const Context& ctxt, const Command* cmds, size_t ncmd);
  inline Transaction(const Context& ctxt, const std::vector<Command>& cmds) :
    Transaction(ctxt, cmds.data(), cmds.size()) {}
  template<size_t N>
  Transaction(const Context& ctxt, const std::array<Command, N>& cmds) :
    Transaction(ctxt, cmds.data(), N) {}
  inline ~Transaction() { destroy_transact(*inner); }

  inline operator HAL_IMPL_NAMESPACE::Transaction& () {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::Transaction& () const {
    return *inner;
  }
};



struct CommandDrain {
  std::unique_ptr<HAL_IMPL_NAMESPACE::CommandDrain> inner;

  CommandDrain(const Context& ctxt);
  inline ~CommandDrain() { destroy_cmd_drain(*inner); }

  inline operator HAL_IMPL_NAMESPACE::CommandDrain& () {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::CommandDrain& () const {
    return *inner;
  }

  inline void submit(const Command* cmds, size_t ncmd) {
    submit_cmds(*inner, cmds, ncmd);
  }
  inline void submit(const std::vector<Command>& cmds) {
    submit(cmds.data(), cmds.size());
  }
  template<size_t N>
  inline void submit(const std::array<Command, N>& cmds) {
    submit(cmds.data(), N);
  }

  inline void wait() {
    wait_cmd_drain(*inner);
  }
};



} // namespace scoped

} // namespace HAL_IMPL_NAMESPACE

} // namespace liong
