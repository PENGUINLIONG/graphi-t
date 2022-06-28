#pragma once
#include "gft/hal/hal.hpp"

#ifndef HAL_IMPL_NAMESPACE
static_assert(false, "please specify the implementation namespace (e.g. `vk`)");
#endif

namespace liong {

namespace HAL_IMPL_NAMESPACE {

namespace scoped {



// Enter a scope of garbage collection so the resources created after this call
// will be released during a next call to `pop_gc_frame`.
extern void push_gc_frame(const std::string& label);
// Release all scoped objects created after a last call to `push_gc_frame`. Mark
// `flatten` true if you want the scoped objects created in this frame to be
// released at the end of the parent frame. The provided `label` should match 
// that passed to `push_gc_frame` of the same scope.
extern void pop_gc_frame(const std::string& label);

// RAII helper to manage GC frame scopes.
struct GcScope {
  std::string label;

  inline GcScope(const std::string& label = "") : label(label) {
    push_gc_frame(label);
  }
  inline ~GcScope() { pop_gc_frame(label); }
};


enum ScopedObjectOwnership {
  // The scoped object is borrowed from an external creator, e.g. another
  // application or the operating system. The scoped objects never attempts to
  // release its `inner`.
  L_SCOPED_OBJECT_OWNERSHIP_BORROWED = 0,
  // The scoped object owns its `inner` but its lifetime is externally
  // controlled, which means that the scoped object follows a RAII pattern. The
  // `inner` is released in the destructor.
  L_SCOPED_OBJECT_OWNERSHIP_OWNED_BY_RAII,
  // The scoped object is borrowed from a GC frame. The `inner` is released when
  // the owner frame is poped.
  L_SCOPED_OBJECT_OWNERSHIP_OWNED_BY_GC_FRAME,
};

#define L_DECLR_SCOPED_OBJ(ty) \
  HAL_IMPL_NAMESPACE::ty* inner; \
  ScopedObjectOwnership ownership; \
  ty() = default; \
  inline ty(HAL_IMPL_NAMESPACE::ty* inner, ScopedObjectOwnership ownership) : \
    inner(inner), ownership(ownership) {} \
  ty(const ty&) = delete; \
  ty(ty&& b) : \
    inner(std::exchange(b.inner, nullptr)), \
    ownership(std::exchange(b.ownership, L_SCOPED_OBJECT_OWNERSHIP_BORROWED)) {} \
  ~ty(); \
  ty& operator=(const ty&) = delete; \
  ty& operator=(ty&& b) { \
    inner = std::exchange(b.inner, nullptr); \
    ownership = std::exchange(b.ownership, L_SCOPED_OBJECT_OWNERSHIP_BORROWED); \
    return *this; \
  } \
  static ty borrow(const HAL_IMPL_NAMESPACE::ty& inner); \
  static ty own_by_raii(HAL_IMPL_NAMESPACE::ty&& inner); \
  static ty own_by_gc_frame(HAL_IMPL_NAMESPACE::ty&& inner); \
  inline operator HAL_IMPL_NAMESPACE::ty&() { return *inner; } \
  inline operator const HAL_IMPL_NAMESPACE::ty&() const { return *inner; }



struct Transaction {
  L_DECLR_SCOPED_OBJ(Transaction);

  inline bool is_done() const {
    return is_transact_done(*this);
  }
  inline void wait() {
    wait_transact(*this);
  }
};



struct Invocation {
  L_DECLR_SCOPED_OBJ(Invocation);

  inline double get_time_us() const {
    return get_invoke_time_us(*this);
  }
  inline void bake() {
    bake_invoke(*this);
  }

  Transaction submit(bool gc = true);
};
struct TransferInvocationBuilder {
  using Self = TransferInvocationBuilder;

  const Context& parent;
  TransferInvocationConfig inner;

  inline TransferInvocationBuilder(
    const Context& ctxt,
    const std::string& label = ""
  ) : parent(ctxt), inner() {
    inner.label = label;
  }

  inline Self& src(const ResourceView& rsc_view) {
    inner.src_rsc_view = rsc_view;
    return *this;
  }
  inline Self& dst(const ResourceView& rsc_view) {
    inner.dst_rsc_view = rsc_view;
    return *this;
  }
  inline Self& is_timed(bool is_timed = true) {
    inner.is_timed = is_timed;
    return *this;
  }

  inline Self& src(const BufferView& buf_view) {
    return src(make_rsc_view(buf_view));
  }
  inline Self& src(const ImageView& img_view) {
    return src(make_rsc_view(img_view));
  }
  inline Self& src(const DepthImageView& depth_img_view) {
    return src(make_rsc_view(depth_img_view));
  }
  inline Self& dst(const BufferView& buf_view) {
    return dst(make_rsc_view(buf_view));
  }
  inline Self& dst(const ImageView& img_view) {
    return dst(make_rsc_view(img_view));
  }
  inline Self& dst(const DepthImageView& depth_img_view) {
    return dst(make_rsc_view(depth_img_view));
  }

  Invocation build(bool gc = true);
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
  inline Self& is_timed(bool is_timed = true) {
    inner.is_timed = is_timed;
    return *this;
  }

  inline Self& rsc(const BufferView& buf_view) {
    return rsc(make_rsc_view(buf_view));
  }
  inline Self& rsc(const ImageView& img_view) {
    return rsc(make_rsc_view(img_view));
  }
  inline Self& rsc(const DepthImageView& depth_img_view) {
    return rsc(make_rsc_view(depth_img_view));
  }

  Invocation build(bool gc = true);
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
  inline Self& idx_ty(IndexType idx_ty) {
    inner.idx_ty = idx_ty;
    return *this;
  }
  inline Self& nidx(uint32_t nidx) {
    inner.nidx = nidx;
    return *this;
  }
  inline Self& is_timed(bool is_timed = true) {
    inner.is_timed = is_timed;
    return *this;
  }

  inline Self& rsc(const BufferView& buf_view) {
    inner.rsc_views.emplace_back(make_rsc_view(buf_view));
    return *this;
  }
  inline Self& rsc(const ImageView& img_view) {
    inner.rsc_views.emplace_back(make_rsc_view(img_view));
    return *this;
  }
  inline Self& rsc(const DepthImageView& depth_img_view) {
    inner.rsc_views.emplace_back(make_rsc_view(depth_img_view));
    return *this;
  }

  Invocation build(bool gc = true);
};
struct RenderPassInvocationBuilder {
  using Self = RenderPassInvocationBuilder;

  const RenderPass& parent;
  RenderPassInvocationConfig inner;

  inline RenderPassInvocationBuilder(
    const RenderPass& pass,
    const std::string& label = ""
  ) : parent(pass), inner() {
    inner.label = label;
  }

  inline Self& attm(const ResourceView& rsc_view) {
    inner.attms.emplace_back(rsc_view);
    return *this;
  }
  inline Self& invoke(const Invocation& invoke) {
    inner.invokes.emplace_back(&(const HAL_IMPL_NAMESPACE::Invocation&)invoke);
    return *this;
  }
  inline Self& is_timed(bool is_timed = true) {
    inner.is_timed = is_timed;
    return *this;
  }

  inline Self& attm(const ImageView& img_view) {
    return attm(make_rsc_view(img_view));
  }
  inline Self& attm(const DepthImageView& depth_img_view) {
    return attm(make_rsc_view(depth_img_view));
  }

  Invocation build(bool gc = true);
};
struct CompositeInvocationBuilder {
  using Self = CompositeInvocationBuilder;

  const Context& parent;
  CompositeInvocationConfig inner;

  inline CompositeInvocationBuilder(
    const Context& ctxt,
    const std::string& label = ""
  ) : parent(ctxt), inner() {
    inner.label = label;
  }

  inline Self& invoke(const Invocation& invoke) {
    inner.invokes.emplace_back(&(const HAL_IMPL_NAMESPACE::Invocation&)invoke);
    return *this;
  }
  inline Self& is_timed(bool is_timed = true) {
    inner.is_timed = is_timed;
    return *this;
  }

  Invocation build(bool gc = true);
};



struct Task {
  L_DECLR_SCOPED_OBJ(Task);

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

  template<typename T>
  inline Self& comp(const std::vector<T>& buf) {
    return comp(buf.data(), buf.size() * sizeof(T));
  }

  Task build(bool gc = true);
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
  inline Self& rsc(const ResourceType& rsc_ty) {
    inner.rsc_tys.emplace_back(rsc_ty);
    return *this;
  }

  template<typename T>
  inline Self& vert(const std::vector<T>& buf) {
    return vert(buf.data(), buf.size() * sizeof(T));
  }
  template<typename T>
  inline Self& frag(const std::vector<T>& buf) {
    return frag(buf.data(), buf.size() * sizeof(T));
  }

  Task build(bool gc = true);
};



struct Image {
  L_DECLR_SCOPED_OBJ(Image);

  inline const ImageConfig& cfg() const {
    return get_img_cfg(*inner);
  }

  inline ImageView view(
    uint32_t x_offset,
    uint32_t y_offset,
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    ImageSampler sampler
  ) const {
    return make_img_view(*inner, x_offset, y_offset, width, height, depth,
      sampler);
  }
  inline ImageView view(
    uint32_t x_offset,
    uint32_t y_offset,
    uint32_t width,
    uint32_t height,
    uint32_t depth
  ) const {
    return view(x_offset, y_offset, width, height, depth,
      L_IMAGE_SAMPLER_LINEAR);
  }
  inline ImageView view(ImageSampler sampler) const {
    const ImageConfig& cfg = get_img_cfg(*inner);
    return view(0, 0, cfg.width, cfg.height, cfg.depth, sampler);
  }
  inline ImageView view() const {
    return view(L_IMAGE_SAMPLER_LINEAR);
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
    inner.height = 0;
    inner.depth = 0;
  }

  inline Self& width(uint32_t width) {
    inner.width = width;
    return *this;
  }
  inline Self& height(uint32_t height) {
    inner.height = height;
    return *this;
  }
  inline Self& depth(uint32_t depth) {
    inner.depth = depth;
    return *this;
  }
  inline Self& fmt(fmt::Format fmt) {
    inner.fmt = fmt;
    return *this;
  }
  inline Self& usage(ImageUsage usage) {
    inner.usage |= usage;
    return *this;
  }

  inline Self& sampled() {
    return usage(L_IMAGE_USAGE_SAMPLED_BIT)
      .usage(L_IMAGE_USAGE_TRANSFER_DST_BIT);
  }
  inline Self& storage() {
    return usage(L_IMAGE_USAGE_STORAGE_BIT)
      .usage(L_IMAGE_USAGE_TRANSFER_SRC_BIT)
      .usage(L_IMAGE_USAGE_TRANSFER_DST_BIT);
  }
  inline Self& attachment() {
    return usage(L_IMAGE_USAGE_ATTACHMENT_BIT)
      .usage(L_IMAGE_USAGE_TRANSFER_SRC_BIT);
  }
  inline Self& subpass_data() {
    return usage(L_IMAGE_USAGE_SUBPASS_DATA_BIT);
  }
  inline Self& tile_memory() {
    return usage(L_IMAGE_USAGE_TILE_MEMORY_BIT);
  }
  inline Self& present() {
    return usage(L_IMAGE_USAGE_PRESENT_BIT)
      .usage(L_IMAGE_USAGE_TRANSFER_DST_BIT);
  }

  Image build(bool gc = true);
};



struct DepthImage {
  L_DECLR_SCOPED_OBJ(DepthImage);

  inline const DepthImageConfig& cfg() const {
    return get_depth_img_cfg(*inner);
  }

  inline DepthImageView view(
    uint32_t x_offset,
    uint32_t y_offset,
    uint32_t width,
    uint32_t height,
    DepthImageSampler sampler
  ) const {
    return make_depth_img_view(*inner, x_offset, y_offset, width, height,
      sampler);
  }
  inline DepthImageView view(
    uint32_t x_offset,
    uint32_t y_offset,
    uint32_t width,
    uint32_t height
  ) const {
    return view(x_offset, y_offset, width, height,
      L_DEPTH_IMAGE_SAMPLER_LINEAR);
  }
  inline DepthImageView view(DepthImageSampler sampler) const {
    const DepthImageConfig& cfg = get_depth_img_cfg(*inner);
    return view(0, 0, cfg.width, cfg.height, sampler);
  }
  inline DepthImageView view() const {
    return view(L_DEPTH_IMAGE_SAMPLER_LINEAR);
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
  inline Self& fmt(fmt::DepthFormat fmt) {
    inner.fmt = fmt;
    return *this;
  }
  inline Self& usage(DepthImageUsage usage) {
    inner.usage |= usage;
    return *this;
  }

  inline Self& sampled() {
    return usage(L_DEPTH_IMAGE_USAGE_SAMPLED_BIT);
  }
  inline Self& attachment() {
    return usage(L_DEPTH_IMAGE_USAGE_ATTACHMENT_BIT);
  }
  inline Self& subpass_data() {
    return usage(L_DEPTH_IMAGE_USAGE_SUBPASS_DATA_BIT);
  }
  inline Self& tile_memory() {
    return usage(L_DEPTH_IMAGE_USAGE_TILE_MEMORY_BIT);
  }

  DepthImage build(bool gc = true);
};



struct Swapchain {
  L_DECLR_SCOPED_OBJ(Swapchain);

  inline const SwapchainConfig& cfg() const {
    return get_swapchain_cfg(*inner);
  }

  Invocation create_present_invoke(bool gc = true) const;

  inline Image get_img() const {
    return Image::borrow(get_swapchain_img(*inner));
  }

  inline uint32_t width() const {
    return get_swapchain_img_width(*inner);
  }
  inline uint32_t height() const {
    return get_swapchain_img_height(*inner);
  }
};
struct SwapchainBuilder {
  using Self = SwapchainBuilder;

  const HAL_IMPL_NAMESPACE::Context& parent;
  SwapchainConfig inner;

  SwapchainBuilder(
    const HAL_IMPL_NAMESPACE::Context& ctxt,
    const std::string& label = ""
  ) : parent(ctxt), inner() {
    inner.label = label;
    inner.nimg = 3;
    inner.fmt = fmt::L_FORMAT_B8G8R8A8_UNORM_PACK32;
    inner.cspace = fmt::L_COLOR_SPACE_SRGB;
  }

  inline Self& nimg(uint32_t nimg) {
    inner.nimg = nimg;
    return *this;
  }
  inline Self& fmt(fmt::Format fmt) {
    inner.fmt = fmt;
    return *this;
  }
  inline Self& cspace(fmt::ColorSpace cspace) {
    inner.cspace = cspace;
    return *this;
  }

  Swapchain build(bool gc = true);
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
  inline void write(const std::vector<T>& src) {
    write(src.data(), src.size() * sizeof(T));
  }
  template<typename T>
  inline void write(const T& src) {
    write(&src, sizeof(T));
  }
};
struct Buffer {
  L_DECLR_SCOPED_OBJ(Buffer);

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
  const void* streaming_data;
  size_t streaming_elem_size;
  size_t streaming_elem_size_aligned;
  size_t nstreaming_elem;

  BufferBuilder(
    const HAL_IMPL_NAMESPACE::Context& ctxt,
    const std::string& label = ""
  ) :
    parent(ctxt),
    inner(),
    streaming_data(),
    streaming_elem_size(),
    streaming_elem_size_aligned(),
    nstreaming_elem()
  {
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
    return usage(L_BUFFER_USAGE_TRANSFER_SRC_BIT)
      .host_access(L_MEMORY_ACCESS_WRITE_BIT);
  }
  inline Self& read_back() {
    return usage(L_BUFFER_USAGE_TRANSFER_DST_BIT)
      .host_access(L_MEMORY_ACCESS_READ_BIT);
  }
  inline Self& uniform() {
    return usage(L_BUFFER_USAGE_TRANSFER_DST_BIT)
      .usage(L_BUFFER_USAGE_UNIFORM_BIT);
  }
  inline Self& storage() {
    return usage(L_BUFFER_USAGE_TRANSFER_SRC_BIT)
      .usage(L_BUFFER_USAGE_TRANSFER_DST_BIT)
      .usage(L_BUFFER_USAGE_STORAGE_BIT);
  }
  inline Self& vertex() {
    return usage(L_BUFFER_USAGE_TRANSFER_DST_BIT)
      .usage(L_BUFFER_USAGE_VERTEX_BIT);
  }
  inline Self& index() {
    return usage(L_BUFFER_USAGE_TRANSFER_DST_BIT)
      .usage(L_BUFFER_USAGE_INDEX_BIT);
  }

  template<typename T>
  inline Self& size_like(const std::vector<T>& data) {
    return size(data.size() * sizeof(T));
  }
  template<typename T>
  inline Self& size_like(const T& data) {
    return size(sizeof(T));
  }

  Self& streaming_with_aligned(
    const void* data,
    size_t elem_size,
    size_t elem_align,
    size_t nelem
  );
  inline Self& streaming_with(const void* data, size_t size) {
    return streaming_with_aligned(data, size, 1, 1);
  }
  template<typename T>
  inline Self& streaming_with(const std::vector<T>& data) {
    return streaming_with(data.data(), data.size() * sizeof(T));
  }
  template<typename T>
  inline Self& streaming_with(const T& data) {
    return streaming_with(&data, sizeof(T));
  }
  template<typename T>
  inline Self& streaming_with_aligned(const std::vector<T>& data, size_t align) {
    return streaming_with_aligned(data.data(), sizeof(T), align, data.size());
  }

  Buffer build(bool gc = true);
};



struct RenderPass {
  L_DECLR_SCOPED_OBJ(RenderPass);

  GraphicsTaskBuilder build_graph_task(const std::string& label = "") const;
  RenderPassInvocationBuilder build_pass_invoke(const std::string& label = "") const;
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
  inline Self& attm(AttachmentAccess access, fmt::Format fmt) {
    AttachmentConfig attm_cfg {};
    attm_cfg.attm_ty = L_ATTACHMENT_TYPE_COLOR;
    attm_cfg.attm_access = access;
    attm_cfg.color_fmt = fmt;

    inner.attm_cfgs.emplace_back(std::move(attm_cfg));
    return *this;
  }
  inline Self& attm(AttachmentAccess access, fmt::DepthFormat fmt) {
    AttachmentConfig attm_cfg {};
    attm_cfg.attm_ty = L_ATTACHMENT_TYPE_DEPTH;
    attm_cfg.attm_access = access;
    attm_cfg.depth_fmt = fmt;

    inner.attm_cfgs.emplace_back(std::move(attm_cfg));
    return *this;
  }

  inline Self& next_subpass() {
    unimplemented();
    return *this;
  }

  inline Self& load_store_attm(fmt::Format fmt) {
    auto access = L_ATTACHMENT_ACCESS_LOAD | L_ATTACHMENT_ACCESS_STORE;
    return attm((AttachmentAccess)access, fmt);
  }
  inline Self& clear_store_attm(fmt::Format fmt) {
    auto access = L_ATTACHMENT_ACCESS_CLEAR | L_ATTACHMENT_ACCESS_STORE;
    return attm((AttachmentAccess)access, fmt);
  }
  inline Self& load_store_attm(fmt::DepthFormat fmt) {
    auto access = L_ATTACHMENT_ACCESS_LOAD | L_ATTACHMENT_ACCESS_STORE;
    return attm((AttachmentAccess)access, fmt);
  }
  inline Self& clear_store_attm(fmt::DepthFormat fmt) {
    auto access = L_ATTACHMENT_ACCESS_CLEAR | L_ATTACHMENT_ACCESS_STORE;
    return attm((AttachmentAccess)access, fmt);
  }

  RenderPass build(bool gc = true);
};



struct Context {
  L_DECLR_SCOPED_OBJ(Context);

  ComputeTaskBuilder build_comp_task(const std::string& label = "") const {
    return ComputeTaskBuilder(*this, label);
  }
  RenderPassBuilder build_pass(const std::string& label = "") const {
    return RenderPassBuilder(*this, label);
  }
  BufferBuilder build_buf(const std::string& label = "") const {
    return BufferBuilder(*this, label);
  }
  ImageBuilder build_img(const std::string& label = "") const {
    return ImageBuilder(*this, label);
  }
  DepthImageBuilder build_depth_img(const std::string& label = "") const {
    return DepthImageBuilder(*this, label);
  }
  SwapchainBuilder build_swapchain(const std::string& label = "") const {
    return SwapchainBuilder(*this, label);
  }
  TransferInvocationBuilder build_trans_invoke(
    const std::string& label = ""
  ) const {
    return TransferInvocationBuilder(*this, label);
  }
  CompositeInvocationBuilder build_composite_invoke(
    const std::string& label = ""
  ) const {
    return CompositeInvocationBuilder(*this, label);
  }
};



} // namespace scoped

} // namespace HAL_IMPL_NAMESPACE

} // namespace liong
