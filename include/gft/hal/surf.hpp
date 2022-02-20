// # Platform surface abstraction layer
// @PENGUINLIONG
//
// This is separated from `hal.hpp` because it interfaces with platform specific
// APIs which is very likely to contaminate namespaces.

#ifndef HAL_IMPL_NAMESPACE
static_assert(false, "please specify the implementation namespace (e.g. `vk`)");
#endif

// Denote a function to be defined by HAL implementation.
#define L_IMPL_FN extern
// Denote a structure to be defined by HAL implementation.
#define L_IMPL_STRUCT

namespace liong {

namespace HAL_IMPL_NAMESPACE {

L_IMPL_STRUCT struct Surface;
L_IMPL_FN void destroy_surf(Surface& surf);

} // namespace HAL_IMPL_NAMESPACE

} // namespace liong



#ifdef _WIN32
#include <Windows.h>

namespace liong {

namespace HAL_IMPL_NAMESPACE {

L_IMPL_FN Surface create_surf(HINSTANCE hinst, HWND hwnd);

} // namespace HAL_IMPL_NAMESPACE

} // namespace liong

#endif // _WIN32



#if defined(ANDROID) || defined(__ANDROID__)

namespace liong {

namespace HAL_IMPL_NAMESPACE {

L_IMPL_FN Surface create_surf(ANativeWindow* wnd);

} // namespace HAL_IMPL_NAMESPACE

} // namespace liong


#endif // defined(ANDROID) || defined(__ANDROID__)
