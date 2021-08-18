#pragma once
#include "hal.hpp"

#ifndef HAL_IMPL_NAMESPACE
static_assert(false, "please specify the implementation namespace (e.g. `vk`)");
#endif

namespace liong {

namespace HAL_IMPL_NAMESPACE {

namespace scoped {

struct Context;
struct Buffer;
struct Image;
struct Task;
struct Framebuffer;
struct ResourcePool;
struct Transaction;
struct CommandDrain;



struct CommandDrain {
  std::unique_ptr<HAL_IMPL_NAMESPACE::CommandDrain> inner;

  CommandDrain(const Context& ctxt);
  CommandDrain(HAL_IMPL_NAMESPACE::CommandDrain&& inner);
  CommandDrain(CommandDrain&&) = default;
  ~CommandDrain();

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



struct Transaction {
  std::unique_ptr<HAL_IMPL_NAMESPACE::Transaction> inner;

  Transaction(const Context& ctxt, const Command* cmds, size_t ncmd);
  Transaction(HAL_IMPL_NAMESPACE::Transaction&& inner);
  Transaction(Transaction&&) = default;
  Transaction(const Context& ctxt, const std::vector<Command>& cmds);
  template<size_t N>
  Transaction(const Context& ctxt, const std::array<Command, N>& cmds) :
    Transaction(ctxt, cmds.data(), N) {}
  ~Transaction();

  inline operator HAL_IMPL_NAMESPACE::Transaction& () {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::Transaction& () const {
    return *inner;
  }
};



struct ResourcePool {
  std::unique_ptr<HAL_IMPL_NAMESPACE::ResourcePool> inner;

  ResourcePool(const Context& ctxt, const Task& task);
  ResourcePool(HAL_IMPL_NAMESPACE::ResourcePool&& inner);
  ResourcePool(ResourcePool&&) = default;
  ~ResourcePool();

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



struct Framebuffer {
  std::unique_ptr<HAL_IMPL_NAMESPACE::Framebuffer> inner;

  Framebuffer(const Context& ctxt, const Task& task, const ImageView& img_view);
  Framebuffer(HAL_IMPL_NAMESPACE::Framebuffer&& inner);
  Framebuffer(Framebuffer&&) = default;
  ~Framebuffer();

  inline operator HAL_IMPL_NAMESPACE::Framebuffer& () {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::Framebuffer& () const {
    return *inner;
  }
};



struct Task {
  std::unique_ptr<HAL_IMPL_NAMESPACE::Task> inner;

  Task(const Context& ctxt, const ComputeTaskConfig& cfg);
  Task(HAL_IMPL_NAMESPACE::Task&& inner);
  Task(Task&&) = default;
  ~Task();

  inline operator HAL_IMPL_NAMESPACE::Task& () {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::Task& () const {
    return *inner;
  }

  ResourcePool create_rsc_pool() const;
  Framebuffer create_framebuf(const ImageView& img_view) const;
};



struct Image {
  std::unique_ptr<HAL_IMPL_NAMESPACE::Image> inner;

  Image(const Context& ctxt, const ImageConfig& cfg);
  Image(HAL_IMPL_NAMESPACE::Image&& inner);
  Image(Image&&) = default;
  ~Image();

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
    return view(0, 0, (uint32_t)img_cfg.nrow, (uint32_t)img_cfg.ncol);
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
MappedBuffer(MappedBuffer&&) = default;
inline ~MappedBuffer() {
  unmap_mem(view, mapped);
}

template<typename T, typename _ = std::enable_if_t<std::is_pointer<T>::value>>
inline operator T() const {
  return (T)mapped;
}
};
struct Buffer {
  std::unique_ptr<HAL_IMPL_NAMESPACE::Buffer> inner;

  Buffer(const Context& ctxt, const BufferConfig& cfg);
  Buffer(HAL_IMPL_NAMESPACE::Buffer&& inner);
  Buffer(Buffer&&) = default;
  ~Buffer();


  inline operator HAL_IMPL_NAMESPACE::Buffer& () {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::Buffer& () const {
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



struct Context {
public:
  std::unique_ptr<HAL_IMPL_NAMESPACE::Context> inner;

  Context(const ContextConfig& cfg);
  Context(const std::string& label, uint32_t dev_idx);
  Context(HAL_IMPL_NAMESPACE::Context&& inner);
  Context(Context&&) = default;
  ~Context();

  inline operator HAL_IMPL_NAMESPACE::Context& () {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::Context& () const {
    return *inner;
  }

  inline const ContextConfig& cfg() const {
    return get_ctxt_cfg(*inner);
  }

  Task create_comp_task(
    const std::string& label,
    const std::string& entry_point,
    const void* code,
    const size_t code_size,
    const std::vector<ResourceConfig>& rsc_cfgs,
    const DispatchSize& workgrp_size
  ) const;
  template<typename T>
  inline Task create_comp_task(
    const std::string& label,
    const std::string& entry_point,
    const std::vector<T>& code,
    const std::vector<ResourceConfig>& rsc_cfgs,
    const DispatchSize& workgrp_size
  ) const {
    return create_comp_task(label, entry_point, code.data(),
      code.size() * sizeof(T), rsc_cfgs, workgrp_size);
  }

  Buffer create_buf(
    const std::string& label,
    MemoryAccess host_access,
    MemoryAccess dev_access,
    size_t size,
    size_t align,
    bool is_const
  ) const;
  Buffer create_const_buf(
    const std::string& label,
    MemoryAccess host_access,
    MemoryAccess dev_access,
    size_t size,
    size_t align = 1
  ) const;
  Buffer create_storage_buf(
    const std::string& label,
    MemoryAccess host_access,
    MemoryAccess dev_access,
    size_t size,
    size_t align = 1
  ) const;

  Image create_img(
    const std::string& label,
    MemoryAccess host_access,
    MemoryAccess dev_access,
    size_t nrow,
    size_t ncol,
    PixelFormat fmt,
    bool is_const
  ) const;
  Image create_sampled_img(
    const std::string& label,
    MemoryAccess host_access,
    MemoryAccess dev_access,
    size_t nrow,
    size_t ncol,
    PixelFormat fmt
  ) const;
  Image create_storage_img(
    const std::string& label,
    MemoryAccess host_access,
    MemoryAccess dev_access,
    size_t nrow,
    size_t ncol,
    size_t pitch,
    PixelFormat fmt
  ) const;

  Transaction create_transact(const std::vector<Command>& cmds) const;

  CommandDrain create_cmd_drain() const;
};

} // namespace scoped

} // namespace HAL_IMPL_NAMESPACE

} // namespace liong
