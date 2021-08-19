// # HAL independent utilities
// @PENGUINLIONG
#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <fstream>

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
  static inline void format_impl(std::stringstream& ss, const T& x, const TArgs& ... others) {
    format_impl_t<T>::format_impl(ss, x);
    format_impl_t<TArgs...>::format_impl(ss, others...);
  }
};

} // namespace

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

} // namespace util

} // namespace liong
