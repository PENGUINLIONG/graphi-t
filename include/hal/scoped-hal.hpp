#pragma once
#include "hal/hal.hpp"

#ifndef HAL_IMPL_NAMESPACE
static_assert(false, "please specify the implementation namespace (e.g. `vk`)");
#endif

namespace liong {

namespace HAL_IMPL_NAMESPACE {

namespace scoped {

struct Context;
struct Buffer;
struct Image;
struct RenderPass;
struct Task;
struct ResourcePool;
struct Transaction;
struct CommandDrain;



struct CommandDrain {
  std::unique_ptr<HAL_IMPL_NAMESPACE::CommandDrain> inner;

  CommandDrain() = default;
  CommandDrain(const Context& ctxt);
  CommandDrain(HAL_IMPL_NAMESPACE::CommandDrain&& inner);
  CommandDrain(CommandDrain&&) = default;
  ~CommandDrain();

  CommandDrain& operator=(CommandDrain&&) = default;

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

  Timestamp() = default;
  Timestamp(const Context& ctxt);
  Timestamp(HAL_IMPL_NAMESPACE::Timestamp&& inner);
  Timestamp(Timestamp&&) = default;
  ~Timestamp();

  Timestamp& operator=(Timestamp&&) = default;

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

  Transaction() = default;
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

  Transaction& operator=(Transaction&&) = default;

  inline operator HAL_IMPL_NAMESPACE::Transaction& () {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::Transaction& () const {
    return *inner;
  }
};



struct Invocation {
  std::unique_ptr<HAL_IMPL_NAMESPACE::Invocation> inner;

  Invocation() = default;
  Invocation(HAL_IMPL_NAMESPACE::Invocation&& inner);
  Invocation(Invocation&&) = default;
  ~Invocation();

  Invocation& operator=(Invocation&&) = default;

  inline operator HAL_IMPL_NAMESPACE::Invocation& () {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::Invocation& () const {
    return *inner;
  }
};
struct ComputeInvocationBuilder {
  using Self = ComputeInvocationBuilder;

  const Task& parent;
  ComputeInvocationConfig inner;

  inline ComputeInvocationBuilder(
    const Task& task,
    const std::string& label = ""
  ) : parent(task), inner() {
    inner.label = label;
    inner.workgrp_count.x = 1;
    inner.workgrp_count.y = 1;
    inner.workgrp_count.z = 1;
  }

  inline Self& rsc(const ResourceView& rsc_view) {
    inner.rsc_views.emplace_back(rsc_view);
    return *this;
  }
  inline Self& workgrp_count(uint32_t x, uint32_t y, uint32_t z) {
    inner.workgrp_count.x = x;
    inner.workgrp_count.y = y;
    inner.workgrp_count.z = z;
    return *this;
  }

  Invocation build();
};
struct GraphicsInvocationBuilder {
  using Self = GraphicsInvocationBuilder;

  const Task& parent;
  GraphicsInvocationConfig inner;

  inline GraphicsInvocationBuilder(
    const Task& task,
    const std::string& label = ""
  ) : parent(task), inner() {
    inner.label = label;
    inner.ninst = 1;
  }

  inline Self& rsc(const ResourceView& rsc_view) {
    inner.rsc_views.emplace_back(rsc_view);
    return *this;
  }
  inline Self& vert_buf(const BufferView& vert_buf) {
    inner.vert_bufs.emplace_back(vert_buf);
    return *this;
  }
  inline Self& nvert(uint32_t nvert) {
    inner.nvert = nvert;
    return *this;
  }
  inline Self& idx_buf(const BufferView& idx_buf) {
    inner.idx_buf = idx_buf;
    return *this;
  }
  inline Self& nidx(uint32_t nidx) {
    inner.nidx = nidx;
    return *this;
  }

  inline Self& rsc(const BufferView& buf_view) {
    inner.rsc_views.emplace_back(buf_view);
    return *this;
  }
  inline Self& rsc(const ImageView& img_view) {
    inner.rsc_views.emplace_back(img_view);
    return *this;
  }

  Invocation build();
};



struct Task {
  std::unique_ptr<HAL_IMPL_NAMESPACE::Task> inner;

  Task() = default;
  Task(HAL_IMPL_NAMESPACE::Task&& inner);
  Task(Task&&) = default;
  ~Task();

  Task& operator=(Task&&) = default;

  inline operator HAL_IMPL_NAMESPACE::Task& () {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::Task& () const {
    return *inner;
  }

  ComputeInvocationBuilder build_comp_invoke(
    const std::string& label = ""
  ) const;
  GraphicsInvocationBuilder build_graph_invoke(
    const std::string& label = ""
  ) const;
};
struct ComputeTaskBuilder {
  using Self = ComputeTaskBuilder;

  const HAL_IMPL_NAMESPACE::Context& parent;
  ComputeTaskConfig inner;

  inline ComputeTaskBuilder(
    const HAL_IMPL_NAMESPACE::Context& ctxt,
    const std::string& label = ""
  ) : parent(ctxt), inner() {
    inner.label = label;
    inner.entry_name = "main";
    inner.workgrp_size.x = 1;
    inner.workgrp_size.y = 1;
    inner.workgrp_size.z = 1;
  }

  inline Self& comp(const void* code, size_t code_size) {
    inner.code = code;
    inner.code_size = code_size;
    return *this;
  }
  inline Self& comp_entry_name(const std::string& entry_name) {
    inner.entry_name = entry_name;
    return *this;
  }
  inline Self& rsc(ResourceType rsc_ty) {
    inner.rsc_tys.emplace_back(rsc_ty);
    return *this;
  }
  inline Self& workgrp_size(uint32_t x, uint32_t y, uint32_t z) {
    inner.workgrp_size.x = x;
    inner.workgrp_size.y = y;
    inner.workgrp_size.z = z;
    return *this;
  }

  template<typename TContainer>
  inline Self& comp(const TContainer& buf) {
    return comp(buf.data(), buf.size() * sizeof(TContainer::value_type));
  }

  Task build();
};
struct GraphicsTaskBuilder {
  using Self = GraphicsTaskBuilder;

  const HAL_IMPL_NAMESPACE::RenderPass& parent;
  GraphicsTaskConfig inner;

  inline GraphicsTaskBuilder(
    const HAL_IMPL_NAMESPACE::RenderPass& pass,
    const std::string& label = ""
  ) : parent(pass), inner() {
    inner.label = label;
    inner.topo = L_TOPOLOGY_TRIANGLE;
    inner.vert_entry_name = "main";
    inner.frag_entry_name = "main";
  }

  inline Self& vert(const void* code, size_t code_size) {
    inner.vert_code = code;
    inner.vert_code_size = code_size;
    return *this;
  }
  inline Self& vert_entry_name(const std::string& entry_name) {
    inner.vert_entry_name = entry_name;
    return *this;
  }
  inline Self& frag(const void* code, size_t code_size) {
    inner.frag_code = code;
    inner.frag_code_size = code_size;
    return *this;
  }
  inline Self& frag_entry_name(const std::string& entry_name) {
    inner.frag_entry_name = entry_name;
    return *this;
  }
  inline Self& topo(Topology topo) {
    inner.topo = topo;
    return *this;
  }
  inline Self& vert_input(PixelFormat fmt, VertexInputRate rate) {
    inner.vert_inputs.emplace_back(VertexInput { fmt, rate });
    return *this;
  }
  inline Self& rsc(const ResourceType& rsc_ty) {
    inner.rsc_tys.emplace_back(rsc_ty);
    return *this;
  }

  template<typename TContainer>
  inline Self& vert(const TContainer& buf) {
    return vert(buf.data(), buf.size() * sizeof(TContainer::value_type));
  }
  template<typename TContainer>
  inline Self& frag(const TContainer& buf) {
    return frag(buf.data(), buf.size() * sizeof(TContainer::value_type));
  }

  inline Self& per_vert_input(PixelFormat fmt) {
    return vert_input(fmt, L_VERTEX_INPUT_RATE_VERTEX);
  }
  inline Self& per_inst_input(PixelFormat fmt) {
    return vert_input(fmt, L_VERTEX_INPUT_RATE_INSTANCE);
  }

  Task build();
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

  constexpr void* data() {
    return buf == nullptr ? mapped : buf;
  }
  constexpr const void* data() const {
    return buf == nullptr ? mapped : buf;
  }

  template<typename T, typename _ = std::enable_if_t<std::is_pointer<T>::value>>
  inline operator T() const {
    return (T)data();
  }

  inline void read(void* dst, size_t size) const {
    std::memcpy(dst, data(), size);
  }
  template<typename T>
  inline void read(std::vector<T>& src) const {
    read(src.data(), src.size() * sizeof(T));
  }
  inline void write(const void* src, size_t size) {
    std::memcpy(data(), src, size);
  }
  template<typename T>
  inline void write(const std::vector<T>& src) {
    write(src.data(), src.size() * sizeof(T));
  }
};

struct Image {
  std::unique_ptr<HAL_IMPL_NAMESPACE::Image> inner;
  bool dont_destroy;

  Image() = default;
  Image(HAL_IMPL_NAMESPACE::Image&& inner);
  Image(Image&&) = default;
  ~Image();

  Image& operator=(Image&&) = default;

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
    uint32_t x_offset,
    uint32_t y_offset,
    uint32_t width,
    uint32_t height
    ) const {
    return make_img_view(*inner, x_offset, y_offset, width, height);
  }
  inline ImageView view() const {
    return make_img_view(*inner);
  }

  inline MappedImage map(
    uint32_t x_offset,
    uint32_t y_offset,
    uint32_t width,
    uint32_t height,
    MemoryAccess map_access
  ) const {
    return MappedImage(view(x_offset, y_offset, width, height), map_access);
  }
  inline MappedImage map(MemoryAccess map_access) const {
    return MappedImage(view(), map_access);
  }
  inline MappedImage map_read() const {
    return map(L_MEMORY_ACCESS_READ_BIT);
  }
  inline MappedImage map_write() const {
    return map(L_MEMORY_ACCESS_WRITE_BIT);
  }
};
struct ImageBuilder {
  using Self = ImageBuilder;

  const HAL_IMPL_NAMESPACE::Context& parent;
  ImageConfig inner;

  ImageBuilder(
    const HAL_IMPL_NAMESPACE::Context& ctxt,
    const std::string& label = ""
  ) : parent(ctxt), inner() {
    inner.label = label;
    inner.width = 1;
    inner.height = 1;
  }

  inline Self& host_access(MemoryAccess access) {
    inner.host_access |= access;
    return *this;
  }
  inline Self& width(uint32_t width) {
    inner.width = width;
    return *this;
  }
  inline Self& height(uint32_t height) {
    inner.height = height;
    return *this;
  }
  inline Self& fmt(PixelFormat fmt) {
    inner.fmt = fmt;
    return *this;
  }
  inline Self& usage(ImageUsage usage) {
    inner.usage |= usage;
    return *this;
  }

  inline Self& streaming() {
    return usage(L_IMAGE_USAGE_STAGING_BIT)
      .host_access(L_MEMORY_ACCESS_WRITE_BIT);
  }
  inline Self& read_back() {
    return usage(L_IMAGE_USAGE_STAGING_BIT)
      .host_access(L_MEMORY_ACCESS_READ_BIT);
  }
  inline Self& sampled() {
    return usage(L_IMAGE_USAGE_SAMPLED_BIT);
  }
  inline Self& storage() {
    return usage(L_IMAGE_USAGE_STORAGE_BIT);
  }
  inline Self& attachment() {
    return usage(L_IMAGE_USAGE_ATTACHMENT_BIT);
  }
  inline Self& present() {
    return usage(L_IMAGE_USAGE_PRESENT_BIT);
  }

  Image build();
};



struct DepthImage {
  std::unique_ptr<HAL_IMPL_NAMESPACE::DepthImage> inner;

  DepthImage(HAL_IMPL_NAMESPACE::DepthImage&& inner);
  DepthImage(DepthImage&&) = default;
  ~DepthImage();

  inline operator HAL_IMPL_NAMESPACE::DepthImage& () {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::DepthImage& () const {
    return *inner;
  }

  inline const DepthImageConfig& cfg() const {
    return get_depth_img_cfg(*inner);
  }
};
struct DepthImageBuilder {
  using Self = DepthImageBuilder;

  const HAL_IMPL_NAMESPACE::Context& parent;
  DepthImageConfig inner;

  DepthImageBuilder(
    const HAL_IMPL_NAMESPACE::Context& ctxt,
    const std::string& label = ""
  ) : parent(ctxt), inner() {
    inner.label = label;
    inner.width = 1;
    inner.height = 1;
  }

  inline Self& width(uint32_t width) {
    inner.width = width;
    return *this;
  }
  inline Self& height(uint32_t height) {
    inner.height = height;
    return *this;
  }
  inline Self& fmt(DepthFormat fmt) {
    inner.fmt = fmt;
    return *this;
  }
  inline Self& usage(DepthImageUsage usage) {
    inner.usage |= usage;
    return *this;
  }

  inline Self& sampled() {
    return usage(L_IMAGE_USAGE_SAMPLED_BIT);
  }
  inline Self& attachment() {
    return usage(L_IMAGE_USAGE_ATTACHMENT_BIT);
  }

  DepthImage build();
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

  constexpr void* data() {
    return mapped;
  }
  constexpr const void* data() const {
    return mapped;
  }

  template<typename T, typename _ = std::enable_if_t<std::is_pointer<T>::value>>
  inline operator T() const {
    return (T)data();
  }

  inline void read(void* dst, size_t size) const {
    std::memcpy(dst, data(), size);
  }
  template<typename T>
  inline void read(std::vector<T>& dst) const {
    read(dst.data(), dst.size() * sizeof(T));
  }
  template<typename T>
  inline void read(T& dst) const {
    read(&dst, sizeof(T));
  }
  inline void write(const void* src, size_t size) {
    std::memcpy(data(), src, size);
  }
  template<typename T>
  inline void write(std::vector<T>& src) {
    write(src.data(), src.size() * sizeof(T));
  }
  template<typename T>
  inline void write(T& src) {
    write(&src, sizeof(T));
  }
};
struct Buffer {
  std::unique_ptr<HAL_IMPL_NAMESPACE::Buffer> inner;
  bool dont_destroy;

  Buffer() = default;
  Buffer(HAL_IMPL_NAMESPACE::Buffer&& inner);
  Buffer(Buffer&&) = default;
  ~Buffer();

  Buffer& operator=(Buffer&&) = default;

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
    return make_buf_view(*inner, offset, size);
  }
  inline BufferView view() const {
    return make_buf_view(*inner);
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
  inline MappedBuffer map_read() const {
    return map(L_MEMORY_ACCESS_READ_BIT);
  }
  inline MappedBuffer map_write() const {
    return map(L_MEMORY_ACCESS_WRITE_BIT);
  }
};
struct BufferBuilder {
  using Self = BufferBuilder;

  const HAL_IMPL_NAMESPACE::Context& parent;
  BufferConfig inner;

  BufferBuilder(
    const HAL_IMPL_NAMESPACE::Context& ctxt,
    const std::string& label = ""
  ) : parent(ctxt), inner() {
    inner.label = label;
    inner.align = 1;
  }

  inline Self& host_access(MemoryAccess access) {
    inner.host_access |= access;
    return *this;
  }
  inline Self& size(size_t size) {
    inner.size = size;
    return *this;
  }
  inline Self& align(size_t align) {
    inner.align = align;
    return *this;
  }
  inline Self& usage(BufferUsage usage) {
    inner.usage |= usage;
    return *this;
  }

  inline Self& streaming() {
    return usage(L_BUFFER_USAGE_STAGING_BIT)
      .host_access(L_MEMORY_ACCESS_WRITE_BIT);
  }
  inline Self& read_back() {
    return usage(L_BUFFER_USAGE_STAGING_BIT)
      .host_access(L_MEMORY_ACCESS_READ_BIT);
  }
  inline Self& uniform() {
    return usage(L_BUFFER_USAGE_UNIFORM_BIT);
  }
  inline Self& storage() {
    return usage(L_BUFFER_USAGE_STORAGE_BIT);
  }
  inline Self& vertex() {
    return usage(L_BUFFER_USAGE_VERTEX_BIT);
  }
  inline Self& index() {
    return usage(L_BUFFER_USAGE_INDEX_BIT);
  }

  Buffer build();
};



struct RenderPass {
public:
  std::unique_ptr<HAL_IMPL_NAMESPACE::RenderPass> inner;

  RenderPass(HAL_IMPL_NAMESPACE::RenderPass&& inner);
  RenderPass(RenderPass&&) = default;
  ~RenderPass();

  inline operator HAL_IMPL_NAMESPACE::RenderPass& () {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::RenderPass& () const {
    return *inner;
  }

  GraphicsTaskBuilder build_graph_task(const std::string& label) const;
};
struct RenderPassBuilder {
  using Self = RenderPassBuilder;

  const HAL_IMPL_NAMESPACE::Context& parent;
  RenderPassConfig inner;

  inline RenderPassBuilder(
    const HAL_IMPL_NAMESPACE::Context& ctxt,
    const std::string& label = ""
  ) : parent(ctxt), inner() {
    inner.label = label;
    inner.width = 1;
    inner.height = 1;
    inner.attm_cfgs.reserve(1);
  }

  inline Self& width(uint32_t width) {
    inner.width = width;
    return *this;
  }
  inline Self& height(uint32_t height) {
    inner.height = height;
    return *this;
  }
  inline Self& attm(AttachmentAccess access, const Image& color_img) {
    AttachmentConfig attm_cfg {};
    attm_cfg.attm_ty = L_ATTACHMENT_TYPE_COLOR;
    attm_cfg.attm_access = access;
    attm_cfg.color_img = &(const HAL_IMPL_NAMESPACE::Image&)color_img;
    inner.attm_cfgs.emplace_back(attm_cfg);
    return *this;
  }
  inline Self& attm(AttachmentAccess access, const DepthImage& depth_img) {
    AttachmentConfig attm_cfg {};
    attm_cfg.attm_ty = L_ATTACHMENT_TYPE_DEPTH;
    attm_cfg.attm_access = access;
    attm_cfg.depth_img = &(const HAL_IMPL_NAMESPACE::DepthImage&)depth_img;
    inner.attm_cfgs.emplace_back(attm_cfg);
    return *this;
  }

  inline Self& load_store_attm(const Image& color_img) {
    auto access = L_ATTACHMENT_ACCESS_LOAD | L_ATTACHMENT_ACCESS_STORE;
    return attm((AttachmentAccess)access, color_img);
  }
  inline Self& clear_store_attm(const Image& color_img) {
    auto access = L_ATTACHMENT_ACCESS_CLEAR | L_ATTACHMENT_ACCESS_STORE;
    return attm((AttachmentAccess)access, color_img);
  }
  inline Self& load_store_attm(const DepthImage& depth_img) {
    auto access = L_ATTACHMENT_ACCESS_LOAD | L_ATTACHMENT_ACCESS_STORE;
    return attm((AttachmentAccess)access, depth_img);
  }
  inline Self& clear_store_attm(const DepthImage& depth_img) {
    auto access = L_ATTACHMENT_ACCESS_CLEAR | L_ATTACHMENT_ACCESS_STORE;
    return attm((AttachmentAccess)access, depth_img);
  }

  RenderPass build();
};


struct Context {
public:
  std::unique_ptr<HAL_IMPL_NAMESPACE::Context> inner;

  Context() = default;
  Context(const ContextConfig& cfg);
  Context(const std::string& label, uint32_t dev_idx);
  Context(HAL_IMPL_NAMESPACE::Context&& inner);
  Context(Context&&) = default;
  ~Context();

  Context& operator=(Context&&) = default;

  inline operator HAL_IMPL_NAMESPACE::Context& () {
    return *inner;
  }
  inline operator const HAL_IMPL_NAMESPACE::Context& () const {
    return *inner;
  }

  inline const ContextConfig& cfg() const {
    return get_ctxt_cfg(*inner);
  }

  ComputeTaskBuilder build_comp_task(const std::string& label = "") const;
  RenderPassBuilder build_pass(const std::string& label = "") const;
  BufferBuilder build_buf(const std::string& label = "") const;
  ImageBuilder build_img(const std::string& label = "") const;
  DepthImageBuilder build_depth_img(const std::string& label = "") const;

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
