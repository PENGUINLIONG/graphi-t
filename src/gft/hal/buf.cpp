#pragma once
#include "gft/hal/buffer.hpp"

namespace liong {
namespace hal {

MappedBuffer::MappedBuffer(const BufferRef &buf, MemoryAccess map_access)
  : buf(buf), mapped(buf->map(map_access)) {}
MappedBuffer::MappedBuffer(MappedBuffer &&x)
  : buf(std::exchange(x.buf, nullptr)),
    mapped(std::exchange(x.mapped, nullptr)) {}
MappedBuffer::~MappedBuffer() {
  if (mapped) {
    buf->unmap();
    mapped = nullptr;
  }
}

} // namespace hal
} // namespace liong
