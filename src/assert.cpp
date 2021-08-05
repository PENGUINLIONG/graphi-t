#include "assert.hpp"

namespace liong {

AssertionFailedException::AssertionFailedException(const std::string& msg) :
  msg(msg) {}
const char* AssertionFailedException::what() const noexcept {
  return msg.c_str();
}

} // namespace liong
