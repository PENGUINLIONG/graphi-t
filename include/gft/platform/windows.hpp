// # Windows platform specific functionalities.
// @PENGUINLIONG
#pragma once
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#undef NOCOMM
#undef WIN32_LEAN_AND_MEAN
#include "gft/assert.hpp"

namespace liong {
namespace windows {

struct Window {
  struct Extra {};
  HINSTANCE hinst;
  HWND hwnd;
};
extern Window create_window(uint32_t width, uint32_t height);
extern Window create_window();

} // namespace windows
} // namespace liong
#endif // _WIN32