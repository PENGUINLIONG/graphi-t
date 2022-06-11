#ifdef _WIN32
#include "gft/platform/windows.hpp"

namespace liong {
namespace windows {

LRESULT wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

Window create_window(uint32_t width, uint32_t height) {
  const uint32_t DEFAULT_WINDOW_X = 200;
  const uint32_t DEFAULT_WINDOW_Y = 100;
  const uint32_t DEFAULT_WINDOW_WIDTH = 1024;
  const uint32_t DEFAULT_WINDOW_HEIGHT = 768;

  if (width == 0) {
    width = DEFAULT_WINDOW_WIDTH;
  }
  if (height == 0) {
    height = DEFAULT_WINDOW_HEIGHT;
  }

  LPSTR MODULE_NAME = TEXT("GraphiT");
  LPSTR WINDOW_CLASS_NAME = TEXT("GraphiTWindowClass");
  LPSTR WINDOW_NAME = TEXT("GraphiTWindow");

  HINSTANCE hinst = GetModuleHandle(MODULE_NAME);

  WNDCLASS wnd_cls;
  wnd_cls.style = CS_HREDRAW | CS_OWNDC | CS_VREDRAW;
  wnd_cls.lpfnWndProc = (WNDPROC)wnd_proc;
  wnd_cls.cbClsExtra = NULL; // No extra
  wnd_cls.cbWndExtra = sizeof(Window::Extra); // window data.
  wnd_cls.hInstance = hinst;
  wnd_cls.hIcon = LoadIcon(NULL, IDI_WINLOGO); // Default icon.
  wnd_cls.hCursor = LoadCursor(NULL, IDC_ARROW); // Default cursor.
  wnd_cls.hbrBackground = NULL;
  wnd_cls.lpszMenuName = NULL;
  wnd_cls.lpszClassName = WINDOW_CLASS_NAME;
  ATOM res = RegisterClass(&wnd_cls);
  L_ASSERT(res, "cannot register window class");

  RECT rect = { 0, 0, width, height };
  DWORD style = WS_OVERLAPPEDWINDOW;
  DWORD exstyle = WS_EX_WINDOWEDGE | WS_EX_APPWINDOW;
  AdjustWindowRectEx(&rect, style, FALSE, exstyle);

  HWND hwnd = CreateWindowEx(exstyle, WINDOW_CLASS_NAME, WINDOW_NAME, style,
    DEFAULT_WINDOW_X, DEFAULT_WINDOW_Y,
    rect.right - rect.left, rect.bottom - rect.top,
    NULL, NULL, hinst, NULL);
  L_ASSERT(hwnd != NULL, "cannot create window");

  ShowWindow(hwnd, SW_SHOW);

  Window out {};
  out.hinst = hinst;
  out.hwnd = hwnd;
  return out;
}
Window create_window() {
  return create_window(0, 0);
}

} // namespace windows
} // namespace liong
#endif // _WIN32
