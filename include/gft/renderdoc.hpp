// RenderDoc Integration, only available on Windows.
// @PENGUINLIONG

namespace liong {

namespace renderdoc {

// Initialize RenderDoc. Repeated calls are silently ignored.
//
// WARNING: This API should be called BEFORE ANY CALL TO HAL `initialize`, or
// RenderDoc would fail to hook the graphics APIs and any attempt to capture
// will fail. Also note that, unlike many other modules in GraphiT, this
// `initialize` is not implicitly called by other functions because a strict
// execution order has to be enforced.
extern void initialize();

// Kick off a capture session and record all commands coming after this.
//
// WARNING: This API should be called AFTER the creation of HAL `Context`, or
// RenderDoc would crash.
extern void begin_capture();
// Stop current capture session and launch RenderDoc GUI.
extern void end_capture();

// An RAII capture guard. Follow the same rule of `begin/end_capture` when using
// this.
struct CaptureGuard {
  inline CaptureGuard() { begin_capture(); }
  inline ~CaptureGuard() { end_capture(); }
};

} // namespace renderdoc

} // namespace liong
