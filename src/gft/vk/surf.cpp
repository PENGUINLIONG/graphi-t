#include "gft/vk.hpp"
#include "gft/log.hpp"

#ifdef _WIN32
typedef void* HINSTANCE;
typedef void* HWND;

#include "vulkan/vulkan_win32.h"

namespace liong {
namespace vk {

VkSurfaceKHR _create_surf(const Context& ctxt, HINSTANCE hinstance, HWND hwnd) {
  VkWin32SurfaceCreateInfoKHR wsci {};
  wsci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  wsci.hinstance = hinstance;
  wsci.hwnd = hwnd;

  VkSurfaceKHR surf;
  VK_ASSERT << vkCreateWin32SurfaceKHR(inst, &wsci, nullptr, &surf);

  return surf;
}

} // namespace vk
} // namespace liong

#else // Surface is not available on this platform.

namespace liong {
namespace vk {

VkSurfaceKHR _create_surf(const Context& ctxt) {
  panic("current platform doesn't support surface creation");
}

} // namespace vk
} // namespace liong

#endif



namespace liong {
namespace vk {

Surface create_surf(const Context& ctxt, const SurfaceConfig& cfg) {
  VkSurfaceKHR surf = _create_surf(ctxt, cfg);

  log::debug("created surface '", cfg.label, "'");
  return Surface { &ctxt, surf, cfg };
}
void destroy_surf(Surface& surf) {
  vkDestroySurfaceKHR(inst, surf.surf, nullptr);
  log::debug("destroyed surface '", surf.surf_cfg.label, "'");
}

} // namespace vk
} // namespace liong
