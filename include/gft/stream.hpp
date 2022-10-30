// In-memory data stream.
// @PENGUINLIONG
#pragma once
#include <cstdint>
#include <vector>
#include <functional>

namespace liong {
namespace stream {

struct ReadStream {
private:
  const void* data_;
  size_t size_;
  size_t offset_;

public:
  inline ReadStream(const void* data, size_t size) :
    data_(data),
    size_(size),
    offset_(0) {}

  inline const void* data() const {
    return data_;
  }
  inline const void* pos() const {
    return (const uint8_t*)data_ + offset_;
  }
  inline size_t size() const {
    return size_;
  }
  // Resetting `offset` is not allowed. Create another `ReadStream` instance
  // instead if necessary.
  inline size_t offset() const {
    return offset_;
  }
  inline size_t size_remain() const {
    return size_ - offset_;
  }
  inline bool ate() const {
    return size_ <= offset_;
  }

  void peek_data(void* out, size_t size);
  void extract_data(void* out, size_t size);

  ReadStream& skip(size_t n);
  template<typename T>
  inline ReadStream& skip() {
    return skip(sizeof(T));
  }


  template<typename T>
  inline T peek() {
    T out {};
    peek_data(&out);
    return out;
  }
  template<typename T>
  inline bool try_peek(T& out) {
    if (size_remain() < sizeof(T)) {
      return false;
    } else {
      peek_data(&out, sizeof(T));
      return true;
    }
  }
  template<typename T>
  inline T extract() {
    T out {};
    extract_data(&out, sizeof(T));
    return out;
  }
  template<typename T>
  inline bool try_extract(T& out) {
    if (size_remain() < sizeof(T)) {
      return false;
    } else {
      extract_data(&out);
      return true;
    }
  }
  template<typename T>
  std::vector<T> extract_all() {
    std::vector<T> out {};
    size_t n = size_remain() / sizeof(T);
    out.resize(n);
    extract_data(out.data(), n * sizeof(T));
    return out;
  }
  template<typename T, typename U>
  std::vector<U> extract_all_map(const std::function<U(const T&)>& f) {
    std::vector<T> tmp {};
    std::vector<U> out {};
    size_t n = size_remain() / sizeof(T);
    tmp.resize(n);
    out.resize(n);
    extract_data(tmp.data(), n * sizeof(T));
    for (size_t i = 0; i < tmp.size(); ++i) {
      out[i] = f(tmp[i]);
    }
    return out;
  }
};

struct WriteStream {
private:
  std::vector<uint8_t> data_;

public:
  inline size_t size() const {
    return data_.size();
  }

  void append_data(const void* data, size_t size);

  template<typename T>
  inline void append(const T& x) {
    append_data(&x, sizeof(T));
  }
  template<typename T>
  inline void append(const std::vector<T>& data) {
    append_data(data.data(), data.size() * sizeof(T));
  }

  inline std::vector<uint8_t> take() {
    return std::move(data_);
  }
};

} // namespace stream
} // namespace liong
