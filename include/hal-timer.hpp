// Simple device-side timer for on-device profiling
// @PENGUINLIONG
#include "hal.hpp"

#ifndef HAL_IMPL_NAMESPACE
static_assert(false, "please specify the implementation namespace (e.g. `vk`)");
#endif

namespace liong {

namespace HAL_IMPL_NAMESPACE {

namespace ext {

struct DeviceTimer {
  Timestamp beg, end;

  inline DeviceTimer(const Context& ctxt) :
    beg(create_timestamp(ctxt)),
    end(create_timestamp(ctxt)) {}
  inline ~DeviceTimer() {
    destroy_timestamp(beg);
    destroy_timestamp(end);
  }

  inline Command cmd_tic() const {
    return cmd_write_timestamp(beg);
  }
  inline Command cmd_toc() const {
    return cmd_write_timestamp(end);
  }

  inline double us() const {
    return get_timestamp_result_us(end) - get_timestamp_result_us(beg);
  }
};

} // namespace ext

} // namespace HAL_IMPL_NAMESPACE

} // namespace liong
