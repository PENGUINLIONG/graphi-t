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



struct Timestamp {
  std::unique_ptr<HAL_IMPL_NAMESPACE::Timestamp> inner;

  Timestamp(const Context& ctxt);
  Timestamp(HAL_IMPL_NAMESPACE::Timestamp&& inner);
  Timestamp(Timestamp&&) = default;
  ~Timestamp();

  inline operator HAL_IMPL_NAMESPACE::Timestamp& () {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::Timestamp& () const {
    return *inner;
  }

  inline double get_result_us() const {
    return HAL_IMPL_NAMESPACE::get_timestamp_result_us(*inner);
  }
};



struct Transaction {
  std::unique_ptr<HAL_IMPL_NAMESPACE::Transaction> inner;

  Transaction(
    const Context& ctxt,
    const std::string& label,
    const Command* cmds, size_t ncmd);
  Transaction(HAL_IMPL_NAMESPACE::Transaction&& inner);
  Transaction(Transaction&&) = default;
  Transaction(
    const Context& ctxt,
    const std::string& label,
    const std::vector<Command>& cmds);
  template<size_t N>
  Transaction(
    const Context& ctxt,
    const std::string& label,
    const std::array<Command, N>& cmds
  ) : Transaction(ctxt, cmds.data(), N) {}
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

  Framebuffer(const Context& ctxt, const Task& task, const Image& attm);
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
  Framebuffer create_framebuf(const Image& attm) const;
};



struct MappedImage {
  void* mapped;
  size_t row_pitch;
  ImageView view;

  void* buf;
  size_t buf_size;

  MappedImage(const ImageView& view, MemoryAccess map_access);
  inline MappedImage(MappedImage&& x) :
    mapped(std::exchange(x.mapped, nullptr)),
    row_pitch(std::exchange(x.row_pitch, 0)),
    view(std::exchange(x.view, {})) {}
  ~MappedImage();

  template<typename T, typename _ = std::enable_if_t<std::is_pointer<T>::value>>
  inline operator T() const {
    if (buf == nullptr) {
      return (T)mapped;
    } else {
      return (T)buf;
    }
  }
};

struct Image {
  std::unique_ptr<HAL_IMPL_NAMESPACE::Image> inner;
  bool dont_destroy;

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
    uint32_t row_offset,
    uint32_t col_offset,
    uint32_t nrow,
    uint32_t ncol
  ) const {
    return ImageView { &*inner, row_offset, col_offset, nrow, ncol };
  }
  inline ImageView view() const {
    auto& img_cfg = cfg();
    return view(0, 0, (uint32_t)img_cfg.nrow, (uint32_t)img_cfg.ncol);
  }

  inline MappedImage map(
    uint32_t row_offset,
    uint32_t col_offset,
    uint32_t ncol,
    uint32_t nrow,
    MemoryAccess map_access
  ) const {
    return MappedImage(view(row_offset, col_offset, nrow, ncol), map_access);
  }
  inline MappedImage map(MemoryAccess map_access) const {
    return MappedImage(view(), map_access);
  }
};



struct MappedBuffer {
  void* mapped;
  BufferView view;

  inline MappedBuffer(const BufferView& view, MemoryAccess map_access) :
    mapped(nullptr),
    view(view)
  {
    map_buf_mem(view, map_access, mapped);
  }
  MappedBuffer(MappedBuffer&& x) :
    mapped(std::exchange(x.mapped, nullptr)),
    view(std::exchange(x.view, {})) {}
  inline ~MappedBuffer() {
    unmap_buf_mem(view, mapped);
  }

  template<typename T, typename _ = std::enable_if_t<std::is_pointer<T>::value>>
  inline operator T() const {
    return (T)mapped;
  }
};
struct Buffer {
  std::unique_ptr<HAL_IMPL_NAMESPACE::Buffer> inner;
  bool dont_destroy;

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
    const DispatchSize& workgrp_size,
    const std::vector<ResourceType>& rsc_tys
  ) const;
  template<typename T>
  inline Task create_comp_task(
    const std::string& label,
    const std::string& entry_point,
    const std::vector<T>& code,
    const DispatchSize& workgrp_size,
    const std::vector<ResourceType>& rsc_tys
  ) const {
    return create_comp_task(label, entry_point, code.data(),
      code.size() * sizeof(T), workgrp_size, rsc_tys);
  }
  Task create_graph_task(
    const std::string& label,
    const std::string& vert_entry_point,
    const void* vert_code,
    const size_t vert_code_size,
    const std::string& frag_entry_point,
    const void* frag_code,
    const size_t frag_code_size,
    Topology topo,
    const std::vector<ResourceType>& rsc_tys
  ) const;
  template<typename T>
  inline Task create_graph_task(
    const std::string& label,
    const std::string& vert_entry_point,
    const std::vector<T>& vert_code,
    const std::string& frag_entry_point,
    const std::vector<T>& frag_code,
    Topology topo,
    const std::vector<ResourceType>& rsc_tys
  ) const {
    return create_graph_task(label, vert_entry_point, vert_code.data(),
      vert_code.size() * sizeof(T), frag_entry_point, frag_code.data(),
      frag_code.size() * sizeof(T), topo, rsc_tys);
  }

  Buffer create_buf(
    const std::string& label,
    BufferUsage usage,
    size_t size,
    size_t align
  ) const;
  Buffer create_staging_buf(
    const std::string& label,
    size_t size,
    size_t align = 1
  ) const;
  Buffer create_uniform_buf(
    const std::string& label,
    size_t size,
    size_t align = 1
  ) const;
  Buffer create_storage_buf(
    const std::string& label,
    size_t size,
    size_t align = 1
  ) const;
  Buffer create_vert_buf(
    const std::string& label,
    size_t size,
    size_t align = 1
  ) const;
  Buffer create_idx_buf(
    const std::string& label,
    size_t size,
    size_t align = 1
  ) const;

  Image create_img(
    const std::string& label,
    ImageUsage usage,
    size_t nrow,
    size_t ncol,
    PixelFormat fmt
  ) const;
  Image create_staging_img(
    const std::string& label,
    size_t nrow,
    size_t ncol,
    PixelFormat fmt
  ) const;
  Image create_sampled_img(
    const std::string& label,
    size_t nrow,
    size_t ncol,
    PixelFormat fmt
  ) const;
  Image create_storage_img(
    const std::string& label,
    size_t nrow,
    size_t ncol,
    PixelFormat fmt
  ) const;
  Image create_attm_img(
    const std::string& label,
    size_t nrow,
    size_t ncol,
    PixelFormat fmt
  ) const;

  Transaction create_transact(
    const std::string& label,
    const std::vector<Command>& cmds
  ) const;

  CommandDrain create_cmd_drain() const;

  Timestamp create_timestamp() const;
};

} // namespace scoped

} // namespace HAL_IMPL_NAMESPACE

} // namespace liong
