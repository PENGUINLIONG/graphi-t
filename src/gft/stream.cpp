#include <cstdlib>
#include "gft/stream.hpp"
#include "gft/assert.hpp"

namespace liong {
namespace stream {

ReadStream& ReadStream::skip(size_t n) {
  L_ASSERT(size_ >= offset_ + n);
  offset_ += n;
  return *this;
}
void ReadStream::extract_data(void* out, size_t size) {
  L_ASSERT(size_remain() >= size);
  const void* buf = (const uint8_t*)data_ + offset_;
  std::memcpy(out, buf, size);
}
void ReadStream::extract_data(void* out, size_t size) {
  peek_data(out, size);
  offset_ += size;
}

void WriteStream::append_data(const void* data, size_t size) {
  size_t offset = data_.size();
  data_.resize(offset + size);
  std::memcpy(data_.data() + offset, data, size);
}

} // namespace stream
} // namespace liong
