// # Assertion
// @PENGUINLIONG
#pragma once
#include "util.hpp"
#undef assert

namespace liong {

class AssertionFailedException : public std::exception {
  std::string msg;
public:
  AssertionFailedException(const std::string& msg);

  const char* what() const noexcept override;
};

template<typename ... TArgs>
inline void assert(bool pred, const TArgs& ... args) {
#ifndef NDEBUG
  if (!pred) {
    throw AssertionFailedException(liong::util::format(args ...));
  }
#endif
}

template<typename ... TArgs>
[[noreturn]] inline void panic(const TArgs& ... args) {
  assert<TArgs ...>(false, args ...);
}
template<typename ... TArgs>
[[noreturn]] inline void unreachable(const TArgs& ... args) {
  assert<const char*, TArgs ...>(false, "reached unreachable code: ", args ...);
}
[[noreturn]] inline void unimplemented() {
  assert<const char*>(false, "reached unimplemented path");
}

} // namespace liong
