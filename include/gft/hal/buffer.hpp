#pragma once
#include "gft/hal/hal.hpp"
#include "gft/log.hpp"

namespace liong {
namespace hal {

struct MappedBuffer {
  BufferRef buf;
  void* mapped;

  MappedBuffer(const BufferRef& buf, MemoryAccess map_access);
  MappedBuffer(MappedBuffer&& x);
  ~MappedBuffer();

  inline void copy_to(void* dst, size_t size) const {
    std::memcpy(dst, mapped, size);
  }
  template<typename T>
  inline void copy_to(T* dst, size_t count) const {
    copy_to(dst, sizeof(T) * count);
  }
  template<typename T>
  inline void copy_to(T* dst, size_t count) {
    copy_to(dst, count);
  }
  template<typename T>
  inline void copy_to(std::vector<T>& dst) const {
    copy_to(dst.data(), dst.size());
  }
  template<typename T>
  inline void copy_to_aligned(T* dst, size_t count, size_t dev_align) {
    for (size_t i = 0; i < count; ++i) {
      std::memcpy(&dst[i], (uint8_t*)mapped + i * dev_align, sizeof(T));
    }
  }

  inline void copy_from(const void* src, size_t size) const {
    std::memcpy(mapped, src, size);
  }
  template<typename T>
  inline void copy_from(const T* src, size_t count) const {
    copy_from(src, sizeof(T) * count);
  }
  template<typename T>
  inline void copy_from(const std::vector<T>& src) const {
    copy_from(src.data(), src.size());
  }
  template<typename T>
  inline void copy_from_aligned(const T* src, size_t count, size_t dev_align) {
    for (size_t i = 0; i < count; ++i) {
      std::memcpy((uint8_t*)mapped + i * dev_align, &src[i], sizeof(T));
    }
  }
};


struct BufferInfo {
  std::string label;
  size_t size;
  MemoryAccess host_access;
  BufferUsage usage;
};
struct Buffer : public std::enable_shared_from_this<Buffer> {
  const BufferInfo info;

  Buffer(BufferInfo&& info) : info(std::move(info)) {}
  virtual ~Buffer() {}

  virtual void* map(MemoryAccess access) = 0;
  virtual void unmap() = 0;

  inline MappedBuffer map_read() {
    return MappedBuffer(shared_from_this(), L_MEMORY_ACCESS_READ_BIT);
  }
  inline MappedBuffer map_write() {
    return MappedBuffer(shared_from_this(), L_MEMORY_ACCESS_WRITE_BIT);
  }
  inline MappedBuffer map_read_write() {
    return MappedBuffer(
      shared_from_this(), L_MEMORY_ACCESS_READ_BIT | L_MEMORY_ACCESS_WRITE_BIT
    );
  }

  inline void copy_to(void* dst, size_t size) {
    if (size == 0) {
      L_WARN("zero-sized copy is ignored");
      return;
    }
    L_ASSERT(info.size >= size, "buffser size is small than dst buffer size");
    this->map_read().copy_to(dst, size);
  }
  template<typename T>
  inline void copy_to(T* dst, size_t count) {
    this->map_read().copy_to(dst, count);
  }
  template<typename T>
  inline void copy_to(std::vector<T>& dst) {
    this->map_read().copy_to(dst);
  }
  template<typename T>
  inline void copy_to(T& dst) {
    this->map_read().copy_to(&dst, sizeof(T));
  }
  template<typename T>
  inline void copy_to_aligned(T* dst, size_t count, size_t dev_align) {
    this->map_read().copy_to_aligned(dst, count, dev_align);
  }

  inline void copy_from(const void* src, size_t size) {
    if (size == 0) {
      L_WARN("zero-sized copy is ignored");
      return;
    }
    L_ASSERT(info.size >= size, "buffser size is small than src buffer size");
    this->map_write().copy_from(src, size);
  }
  template<typename T>
  inline void copy_from(const T* src, size_t count) {
    this->map_write().copy_from(src, count);
  }
  template<typename T>
  inline void copy_from(const std::vector<T>& src) {
    this->map_write().copy_from(src);
  }
  template<typename T>
  inline void copy_from(const T& src) {
    this->map_write().copy_from(&src, sizeof(T));
  }
  template<typename T>
  inline void copy_from_aligned(const T* src, size_t count, size_t dev_align) {
    this->map_write().copy_from_aligned(src, count, dev_align);
  }

  inline BufferView view(size_t offset, size_t size) {
    BufferView out {};
    out.buf = shared_from_this();
    out.offset = offset;
    out.size = size;
    return out;
  }
  inline BufferView view() {
    return this->view(0, info.size);
  }
};

} // namespace hal
} // namespace liong
