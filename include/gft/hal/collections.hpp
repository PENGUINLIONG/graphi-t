// GPU data collections.
// @PENGUINLIONG
#pragma once
#include "gft/hal/scoped-hal.hpp"

#ifndef HAL_IMPL_NAMESPACE
static_assert(false, "please specify the implementation namespace (e.g. `vk`)");
#endif

namespace liong {
namespace HAL_IMPL_NAMESPACE {
namespace scoped {

template<typename T>
struct BufferArray {
private:
  typedef BufferArray<T> Self;

  struct Inner {
    Buffer buf;
    std::vector<size_t> shape;
    size_t count; // Product of elements in `shape`.
    bool host_access;
  };
  std::shared_ptr<Inner> inner_;

  static BufferArray create_(
    const Context& ctxt,
    const std::vector<size_t>& shape,
    bool host_access,
    BufferUsage usage
  ) {
    size_t count = 1;
    for (size_t i = 0; i < shape.size(); ++i) {
      count *= shape.at(i);
    }
    usage |=
      L_BUFFER_USAGE_STORAGE_BIT |
      L_BUFFER_USAGE_TRANSFER_SRC_BIT |
      L_BUFFER_USAGE_TRANSFER_DST_BIT;
    MemoryAccess host_access2 = 0;
    if (host_access) {
      host_access2 = L_MEMORY_ACCESS_READ_BIT | L_MEMORY_ACCESS_WRITE_BIT;
    }

    auto inner = std::make_shared<Inner>();
    inner->buf = ctxt.build_buf()
      .size(count * sizeof(T))
      .usage(usage)
      .host_access(host_access2)
      .build();
    inner->shape = shape;
    inner->count = count;
    inner->host_access = host_access;

    BufferArray out {};
    out.inner_ = std::move(inner);
    return out;
  }

public:
  static Self create(
    const Context& ctxt,
    const std::vector<size_t>& shape,
    bool host_access = false
  ) {
    return create_(ctxt, shape, host_access, 0);
  }
  static Self create_vertex_buffer(
    const Context& ctxt,
    const size_t nvert,
    bool host_access = false
  ) {
    return create_(ctxt, {nvert}, host_access, L_BUFFER_USAGE_VERTEX_BIT);
  }
  static Self create_index_buffer(
    const Context& ctxt,
    const size_t nidx,
    bool host_access = false
  ) {
    return create_(ctxt, {nidx}, host_access, L_BUFFER_USAGE_INDEX_BIT);
  }

  inline BufferView view() const {
    return inner_->buf.view();
  }

  inline size_t count() const {
    return inner_->count;
  }
  inline BufferUsage usage() const {
    return inner_->buf.usage();
  }

  inline void read(std::vector<T>& dst) {
    inner_->buf.map_read().read(dst);
  }
  inline void write(const std::vector<T>& src) {
    inner_->buf.map_write().write(src);
  }

};

} // namespace scoped
} // namespace HAL_IMPL_NAMESPACE
} // namespace liong
