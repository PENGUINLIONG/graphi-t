// #macOS platform specific functionalities.
// @PENGUINLIONG
#pragma once
#if defined(__MACH__) && defined(__APPLE__)
#include <cstdint>
#include <type_traits>

namespace liong {
namespace macos {

#define NSWindow void
#define CAMetalLayer void

struct Window {
  NSWindow* window;
  CAMetalLayer* metal_layer;
};

Window create_window(uint32_t width, uint32_t height);
Window create_window();

#undef NSWindow
#undef CAMetalLayer

} // namespace macos
} // namespace liong
#endif // defined(__MACH__) && defined(__APPLE__)
