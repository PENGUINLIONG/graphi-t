// Simple timer used for profiling CPU procedures.
// @PENGUINLIONG
#include <chrono>

namespace liong {

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

} // namespace liong
