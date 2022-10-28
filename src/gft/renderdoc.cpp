#include <memory>
#include "gft/assert.hpp"
#include "gft/util.hpp"
#include "gft/log.hpp"
#include "gft/renderdoc.hpp"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define RENDERDOC_CC __cdecl
#else
#define RENDERDOC_CC
#endif // _WIN32


// https://github.com/baldurk/renderdoc/blob/v1.x/renderdoc/api/app/renderdoc_app.h
typedef void* RENDERDOC_IgnoredApi;
typedef void* RENDERDOC_IgnoredHandle;
typedef enum RENDERDOC_Version {
  eRENDERDOC_API_Version_1_0_0 = 10000,    // RENDERDOC_API_1_0_0 = 1 00 00
} RENDERDOC_Version;

typedef int(RENDERDOC_CC *pRENDERDOC_GetAPI)(RENDERDOC_Version version, void **outAPIPointers);

typedef uint32_t(RENDERDOC_CC *pRENDERDOC_GetNumCaptures)();
typedef uint32_t(RENDERDOC_CC *pRENDERDOC_GetCapture)(
  uint32_t idx,
  char *filename,
  uint32_t *pathlength,
  uint64_t *timestamp
);
typedef uint32_t(RENDERDOC_CC *pRENDERDOC_LaunchReplayUI)(
  uint32_t connectTargetControl,
  const char *cmdline
);

typedef void(RENDERDOC_CC *pRENDERDOC_StartFrameCapture)(
  RENDERDOC_IgnoredHandle device,
  RENDERDOC_IgnoredHandle wndHandle
);
typedef uint32_t(RENDERDOC_CC *pRENDERDOC_EndFrameCapture)(
  RENDERDOC_IgnoredHandle device,
  RENDERDOC_IgnoredHandle wndHandle
);

typedef struct RENDERDOC_API_1_0_0
{
  RENDERDOC_IgnoredApi GetAPIVersion;

  RENDERDOC_IgnoredApi SetCaptureOptionU32;
  RENDERDOC_IgnoredApi SetCaptureOptionF32;

  RENDERDOC_IgnoredApi GetCaptureOptionU32;
  RENDERDOC_IgnoredApi GetCaptureOptionF32;

  RENDERDOC_IgnoredApi SetFocusToggleKeys;
  RENDERDOC_IgnoredApi SetCaptureKeys;

  RENDERDOC_IgnoredApi GetOverlayBits;
  RENDERDOC_IgnoredApi MaskOverlayBits;

  RENDERDOC_IgnoredApi RemoveHooks;
  RENDERDOC_IgnoredApi UnloadCrashHandler;

  RENDERDOC_IgnoredApi SetCaptureFilePathTemplate;
  RENDERDOC_IgnoredApi GetCaptureFilePathTemplate;

  pRENDERDOC_GetNumCaptures GetNumCaptures;
  pRENDERDOC_GetCapture GetCapture;

  RENDERDOC_IgnoredApi TriggerCapture;

  RENDERDOC_IgnoredApi IsTargetControlConnected;
  pRENDERDOC_LaunchReplayUI LaunchReplayUI;

  RENDERDOC_IgnoredApi SetActiveWindow;

  pRENDERDOC_StartFrameCapture StartFrameCapture;
  RENDERDOC_IgnoredApi IsFrameCapturing;
  pRENDERDOC_EndFrameCapture EndFrameCapture;
} RENDERDOC_API_1_0_0;



namespace liong {

namespace renderdoc {

struct Context {
  RENDERDOC_API_1_0_0* api;
  Context(RENDERDOC_API_1_0_0* api) :
    api(api) {}
  virtual ~Context() {};

  virtual void begin_capture() {
    api->StartFrameCapture(nullptr, nullptr);
  };
  virtual void end_capture() {
    if (api->EndFrameCapture(nullptr, nullptr) != 1) {
      L_WARN("renderdoc failed to capture this scoped frame");
      return;
    }

    // Kick off the GUI.
    std::string path;
    uint32_t size = 1024;
    path.resize(size);
    uint32_t icapture = api->GetNumCaptures() - 1;
    if (api->GetCapture(icapture, (char*)path.data(), &size, nullptr) != 1) {
      L_WARN("cannot get renderdoc capture path, will not launch replay ui "
        "for this one");
    } else {
      api->LaunchReplayUI(1, path.c_str());
      L_INFO("launched renderdoc replay ui for captured frame #", icapture);
    }
  };
};
std::shared_ptr<Context> ctxt = nullptr;

#ifdef _WIN32
struct WindowsContext : public Context {
  HMODULE mod;
  bool should_release;
  WindowsContext(RENDERDOC_API_1_0_0* api, HMODULE mod, bool should_release) :
    Context(api), mod(mod), should_release(should_release) {}
  virtual ~WindowsContext() override final {
    if (should_release) {
      FreeLibrary(mod);
      L_INFO("renderdoc is unloaded");
    }
  }
};
#endif // _WIN32



static bool is_first_call = true;
void initialize() {
  if (ctxt != nullptr) { return; }

  if (!is_first_call) { return; }
  is_first_call = false;

#ifdef _WIN32
  {
    HMODULE mod = GetModuleHandleA("renderdoc.dll");
    bool should_release;
    if (mod != NULL) {
      should_release = false;
    } else {
      // Find renderdoc on local disk.
      std::string path;
      LSTATUS err = ERROR_MORE_DATA;
      while (err == ERROR_MORE_DATA && path.size() < 2048) {
        path.resize(path.size() + 256);
        DWORD cap = path.size();
        //HKEY reg_key;
        //RegOpenKeyA(HKEY_LOCAL_MACHINE,
        //  "SOFTWARE\\Classes\\RenderDoc.RDCCapture.1\\DefaultIcon", &reg_key);
        err = RegGetValueA(HKEY_LOCAL_MACHINE,
          "SOFTWARE\\Classes\\RenderDoc.RDCCapture.1\\DefaultIcon\\", "",
          RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, NULL,
          (void*)path.data(), &cap);
      }
      if (err != ERROR_SUCCESS || path.empty()) {
        L_WARN("failed to find renderdoc on local installtion, renderdoc "
          "utils become nops");
        return;
      } else {
        // Rewrite the path to renderdoc.
        path.resize(std::strlen(path.c_str()));
        if (util::ends_with("qrenderdoc.exe", path)) {
          path.resize(path.size() - 14);
        } else {
          path.clear();
        }
        path += "renderdoc.dll";
      }

      mod = LoadLibraryA(path.c_str());
      if (mod == NULL) {
        L_WARN("failed to load renderdoc library from installtion, "
          "renderdoc utils become nops");
        return;
      }
      should_release = true;
    }

    auto get_api = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
    RENDERDOC_API_1_0_0* api;
    if (
      get_api == nullptr ||
      get_api(eRENDERDOC_API_Version_1_0_0, (void**)&api) != 1 ||
      api == nullptr
    ) {
      L_WARN("failed to get renderdoc api from running instance, renderdoc "
        "apis become nops");
      return;
    } else {
      ctxt = std::shared_ptr<Context>(
        (Context*)new WindowsContext(api, mod, should_release));
    }
  }
#else // _WIN32
  {
    L_WARN("renderdoc is not supported on current platform, renderdoc utils "
      "become nops");
  }
#endif // _WIN32

  if (ctxt) {
    L_INFO("renderdoc is ready to capture");
  }
}

bool is_captureing = false;
void begin_capture() {
  L_ASSERT(!is_captureing, "cannot begin capture session inside another");
  is_captureing = true;
  if (ctxt == nullptr) {
    if (is_first_call) {
      panic("renderdoc must be initialized before any capture");
    } else {
      L_WARN("frame capture is attempted but it will be ignored because "
        "renderdoc failed to initialize");
    }
  } else {
    ctxt->begin_capture();
  }
}
void end_capture() {
  L_ASSERT(is_captureing, "cannot end a capture out of any session");
  is_captureing = false;
  if (ctxt == nullptr) {
    if (is_first_call) {
      panic("renderdoc must be initialized before any capture");
    }
  } else {
    ctxt->end_capture();
  }
}

} // namespace renderdoc

} // namespace liong
