// # Assertion
// @PENGUINLIONG
#pragma once
#include "gft/util.hpp"
#undef assert

namespace liong {

class AssertionFailedException : public std::exception {
  std::string msg;
public:
  inline AssertionFailedException(const std::string& msg) :
    msg(msg) {}

  const char* what() const noexcept override {
    return msg.c_str();
  }
};

template<typename ... TArgs>
inline void assert(bool pred, const TArgs& ... args) {
#ifndef NDEBUG
  if (!pred) {
    throw AssertionFailedException(util::format(args ...));
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
