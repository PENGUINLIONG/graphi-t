// # HAL independent utilities
// @PENGUINLIONG
#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <functional>

namespace liong {

namespace util {

namespace {

template<typename ... TArgs>
struct format_impl_t;
template<>
struct format_impl_t<> {
  static inline void format_impl(std::stringstream& ss) {}
};
template<typename T>
struct format_impl_t<T> {
  static inline void format_impl(std::stringstream& ss, const T& x) {
    ss << x;
  }
};
template<typename T, typename ... TArgs>
struct format_impl_t<T, TArgs ...> {
  static inline void format_impl(
    std::stringstream& ss,
    const T& x,
    const TArgs& ... others
  ) {
    format_impl_t<T>::format_impl(ss, x);
    format_impl_t<TArgs...>::format_impl(ss, others...);
  }
};

} // namespace

template<typename T, size_t N>
std::string join(const std::string& sep, const std::array<T, N>& strs) {
  std::stringstream ss {};
  bool first = true;
  for (const auto& str : strs) {
    if (first) {
      first = false;
    } else {
      ss << sep;
    }
    ss << str;
  }
  return ss.str();
}
template<typename T>
std::string join(const std::string& sep, const std::vector<T>& strs) {
  std::stringstream ss {};
  bool first = true;
  for (const auto& str : strs) {
    if (first) {
      first = false;
    } else {
      ss << sep;
    }
    ss << str;
  }
  return ss.str();
}
template<typename ... TArgs>
inline std::string format(const TArgs& ... args) {
  std::stringstream ss {};
  format_impl_t<TArgs...>::format_impl(ss, args...);
  return ss.str();
}


extern std::vector<uint8_t> load_file(const char* path);
extern std::string load_text(const char* path);
extern void save_file(const char* path, const void* data, size_t size);

void save_bmp(
  const uint32_t* pxs,
  uint32_t w,
  uint32_t h,
  const char* path
);
void save_bmp(
  const float* pxs,
  uint32_t w,
  uint32_t h,
  const char* path
);


template<typename T>
T count_set_bits(T bitset) {
  T count = 0;
  for (T i = 0; i < sizeof(T) * 8; ++i) {
    if (((bitset >> i) & 1) != 0) {
      ++count;
    }
  }
  return count;
}
template<typename T>
T count_clear_bits(T bitset) {
  T count = 0;
  for (T i = 0; i < sizeof(T) * 8; ++i) {
    if (((bitset >> i) & 1) == 0) {
      ++count;
    }
  }
  return count;
}

template<typename T>
std::vector<T> arrange(T a, T b, T step) {
  std::vector<T> out;
  for (T i = a; i < b; i += step) {
    out.emplace_back(i);
  }
  return out;
}
template<typename T>
std::vector<T> arrange(T a, T b) {
  return arrange<T>(a, b, 1);
}
template<typename T>
std::vector<T> arrange(T b) {
  return arrange<T>(0, b);
}

template<typename T, typename U>
std::vector<U> map(const std::vector<T>& xs, const std::function<U(const T&)>& f) {
  std::vector<U> out;
  out.reserve(xs.size());
  for (const auto& x : xs) {
    out.emplace_back(f(x));
  }
  return out;
}

void sleep_for_us(uint64_t t);

} // namespace util

} // namespace liong
