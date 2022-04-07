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
#include <chrono>

namespace liong {

namespace util {

// - [String Processing] -------------------------------------------------------

bool starts_with(const std::string& start, const std::string& str);
bool ends_with(const std::string& end, const std::string& str);
std::vector<std::string> split(char sep, const std::string& str);
std::string trim(const std::string& str);

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
  static inline void join_impl(
    std::stringstream& ss,
    const std::string& sep,
    const T& x,
    const TArgs& ... others
  ) {
    format_impl_t<T>::format_impl(ss, x);
    ss << sep;
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
inline std::string join(const std::string& sep, const TArgs& ... args) {
  std::stringstream ss {};
  format_impl_t<TArgs...>::join_impl(ss, sep, args...);
  return ss.str();
}
template<typename ... TArgs>
inline std::string format(const TArgs& ... args) {
  std::stringstream ss {};
  format_impl_t<TArgs...>::format_impl(ss, args...);
  return ss.str();
}

// - [File I/O] ----------------------------------------------------------------

extern std::vector<uint8_t> load_file(const char* path);
extern std::string load_text(const char* path);
extern void save_file(const char* path, const void* data, size_t size);
extern void save_text(const char* path, const std::string& txt);

void save_bmp(const uint32_t* pxs, uint32_t w, uint32_t h, const char* path);
void save_bmp(const float* pxs, uint32_t w, uint32_t h, const char* path);

// - [Bitfield Manipulation] ---------------------------------------------------

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

// - [Data Transformation] -----------------------------------------------------

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
std::vector<U> map(
  const std::vector<T>& xs,
  const std::function<U(const T&)>& f
) {
  std::vector<U> out;
  out.reserve(xs.size());
  for (const auto& x : xs) {
    out.emplace_back(f(x));
  }
  return out;
}

template<typename T>
std::vector<T> reinterpret_data(const void* data, size_t size) {
  std::vector<T> out;
  //assert(size % sizeof(T) == 0,
  //  "cannot reinterpret data with size not aligned to the given type");
  out.resize(size / sizeof(T));
  std::memcpy(out.data(), data, size);
  return out;
}
template<typename T, typename U>
std::vector<U> reinterpret_data(const std::vector<T>& x) {
  return reinterpret_data<U>(x.data(), x.size());
}

struct DataStream {
  const void* borrowed;
  size_t size;
  std::vector<uint8_t> owned;
  size_t offset;

  inline DataStream(const void* borrowed, size_t size) :
    borrowed(borrowed),
    size(size),
    owned(),
    offset(0) {}
  inline DataStream(std::vector<uint8_t>&& owned) :
    borrowed(nullptr),
    size(owned.size()),
    owned(std::forward<std::vector<uint8_t>>(owned)),
    offset(0) {}

  constexpr const void* data() const {
    return borrowed != nullptr ? borrowed : owned.data();
  }
  constexpr size_t size_remain() const {
    return size - offset;
  }
  constexpr bool ate() const {
    return size_remain() <= 0;
  }

  void extract_data(void* out, size_t size);

  DataStream& skip(size_t n);
  template<typename T>
  inline DataStream& skip() {
    return skip(sizeof(T));
  }

  template<typename T>
  const T& extract() {
    T out {};
    extract_data(&out);
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

// - [Timing & Temporal Control] -----------------------------------------------

struct Timer {
  std::chrono::time_point<std::chrono::high_resolution_clock> beg, end;

  inline void tic() {
    beg = std::chrono::high_resolution_clock::now();
  }
  inline void toc() {
    end = std::chrono::high_resolution_clock::now();
  }

  inline double us() const {
    std::chrono::duration<double, std::micro> dt = end - beg;
    return dt.count();
  }
};

void sleep_for_us(uint64_t t);

// - [Index & Size Manipulation] -----------------------------------------------

constexpr size_t div_down(size_t x, size_t align) {
  return x / align;
}
constexpr size_t div_up(size_t x, size_t align) {
  return div_down(x + (align - 1), align);
}
constexpr size_t align_down(size_t x, size_t align) {
  return div_down(x, align) * align;
}
constexpr size_t align_up(size_t x, size_t align) {
  return div_up(x, align) * align;
}

constexpr void push_idx(size_t& aggr_idx, size_t i, size_t dim_size) {
  aggr_idx = aggr_idx * dim_size + i;
}
constexpr size_t pop_idx(size_t& aggr_idx, size_t dim_size) {
  size_t out = aggr_idx % dim_size;
  aggr_idx /= dim_size;
  return out;
}

} // namespace util

} // namespace liong
