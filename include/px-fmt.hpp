// Color and depth-stencil pixel format specification.
// @PENGUINLIONG
#pragma once
#include <cstdint>
#include "assert.hpp"

#ifndef HAL_IMPL_NAMESPACE
static_assert(false, "please specify the implementation namespace (e.g. `vk`)");
#endif

namespace liong {

namespace HAL_IMPL_NAMESPACE {

// - [Color Formats] -----------------------------------------------------------

// Encoded pixel format that can be easily decoded by shift-and ops.
//
//   0bccshibbb
//       \____/
//  `CUarray_format`
//
// - `b`: Number of bits in exponent of 2. Only assigned if its a integral
//   number.
// - `i`: Signedness of integral number.
// - `h`: Is a half-precision floating-point number.
// - `s`: Is a single-precision floating-point number.
// - `c`: Number of texel components (channels) minus 1. Currently only support
//   upto 4 components.
struct PixelFormat {
  union {
    struct {
      uint8_t int_exp2 : 3;
      uint8_t is_signed : 1;
      uint8_t is_half : 1;
      uint8_t is_single : 1;
      uint8_t ncomp : 2;
    };
    uint8_t _raw;
  };
  constexpr PixelFormat(uint8_t raw) : _raw(raw) {}
  constexpr PixelFormat() : _raw() {}
  PixelFormat(const PixelFormat&) = default;
  PixelFormat(PixelFormat&&) = default;
  PixelFormat& operator=(const PixelFormat&) = default;
  PixelFormat& operator=(PixelFormat&&) = default;

  constexpr uint32_t get_ncomp() const { return ncomp + 1; }
  constexpr uint32_t get_fmt_size() const {
    uint32_t comp_size = 0;
    if (is_single) {
      comp_size = sizeof(float);
    } else if (is_half) {
      comp_size = sizeof(uint16_t);
    } else {
      comp_size = 4 << (int_exp2) >> 3;
    }
    return get_ncomp() * comp_size;
  }
  // Extract a real number from the buffer.
  inline float extract(const void* buf, size_t i, uint32_t comp) const {
    if (comp > ncomp) { return 0.f; }
    if (is_single) {
      return ((const float*)buf)[i * get_ncomp() + comp];
    } else if (is_half) {
      panic("fp16 extraction not implemented yet");
    } else if (is_signed) {
      switch (int_exp2) {
      case 1:
        return ((const int8_t*)buf)[i * get_ncomp() + comp] / 128.f;
      case 2:
        return ((const int16_t*)buf)[i * get_ncomp() + comp] / 32768.f;
      case 3:
        return ((const int32_t*)buf)[i * get_ncomp() + comp] / 2147483648.f;
      }
    } else {
      switch (int_exp2) {
      case 1:
        return ((const uint8_t*)buf)[i * get_ncomp() + comp] / 255.f;
      case 2:
        return ((const uint16_t*)buf)[i * get_ncomp() + comp] / 65535.f;
      case 3:
        return ((const uint32_t*)buf)[i * get_ncomp() + comp] / 4294967296.f;
      }
    }
    panic("unsupported framebuffer format");
    return 0.0f;
  }
  // Extract a 32-bit word from the buffer as an integer. If the format is
  // shorter than 32 bits zeros are padded from MSB.
  inline uint32_t extract_word(const void* buf, size_t i, uint32_t comp) const {
    assert(!is_single & !is_half,
      "real number type cannot be extracted as bitfield");
    switch (int_exp2) {
    case 1:
      return ((const uint8_t*)buf)[i * get_ncomp() + comp];
    case 2:
      return ((const uint16_t*)buf)[i * get_ncomp() + comp];
    case 3:
      return ((const uint32_t*)buf)[i * get_ncomp() + comp];
    }
    panic("unsupported framebuffer format");
    return 0;
  }
  constexpr bool operator==(const PixelFormat& b) const {
    return _raw == b._raw;
  }
};
#define L_MAKE_VEC(ncomp, fmt) \
  ((uint8_t)((ncomp - 1) << 6) | (uint8_t)fmt)
#define L_DEF_FMT(name, ncomp, fmt) \
  constexpr PixelFormat L_FORMAT_##name = PixelFormat(L_MAKE_VEC(ncomp, fmt))
L_DEF_FMT(UNDEFINED, 1, 0x00);

L_DEF_FMT(R8_UNORM, 1, 0x01);
L_DEF_FMT(R8G8_UNORM, 2, 0x01);
L_DEF_FMT(R8G8B8_UNORM, 3, 0x01);
L_DEF_FMT(R8G8B8A8_UNORM, 4, 0x01);

L_DEF_FMT(R8_UINT, 1, 0x01);
L_DEF_FMT(R8G8_UINT, 2, 0x01);
L_DEF_FMT(R8G8B8_UINT, 3, 0x01);
L_DEF_FMT(R8G8B8A8_UINT, 4, 0x01);

L_DEF_FMT(R16_UINT, 1, 0x02);
L_DEF_FMT(R16G16_UINT, 2, 0x02);
L_DEF_FMT(R16G16B16_UINT, 3, 0x02);
L_DEF_FMT(R16G16B16A16_UINT, 4, 0x02);

L_DEF_FMT(R16_SFLOAT, 1, 0x10);
L_DEF_FMT(R16G16_SFLOAT, 2, 0x10);
L_DEF_FMT(R16G16B16_SFLOAT, 3, 0x10);
L_DEF_FMT(R16G16B16A16_SFLOAT, 4, 0x10);

L_DEF_FMT(R32_UINT, 1, 0x03);
L_DEF_FMT(R32G32_UINT, 2, 0x03);
L_DEF_FMT(R32G32B32_UINT, 3, 0x03);
L_DEF_FMT(R32G32B32A32_UINT, 4, 0x03);

L_DEF_FMT(R32_SFLOAT, 1, 0x20);
L_DEF_FMT(R32G32_SFLOAT, 2, 0x20);
L_DEF_FMT(R32G32B32_SFLOAT, 3, 0x20);
L_DEF_FMT(R32G32B32A32_SFLOAT, 4, 0x20);
#undef L_DEF_FMT
#undef L_MAKE_VEC

// - [Depth-stencil Formats] ---------------------------------------------------

struct DepthFormat {
  uint8_t nbit_depth;
  uint8_t nbit_stencil;
};

#define L_DEF_DEPTH_FORMAT(nbit_depth, nbit_stencil) \
  constexpr DepthFormat L_DEPTH_FORMAT_D##nbit_depth##_S##nbit_stencil = \
    DepthFormat { nbit_depth, nbit_stencil };
L_DEF_DEPTH_FORMAT(16, 0)
L_DEF_DEPTH_FORMAT(24, 0)
L_DEF_DEPTH_FORMAT(32, 0)
L_DEF_DEPTH_FORMAT(0, 8)
L_DEF_DEPTH_FORMAT(16, 8)
L_DEF_DEPTH_FORMAT(24, 8)
L_DEF_DEPTH_FORMAT(32, 8)
#undef L_DEF_DEPTH_FORMAT

} // namespace 

} // namespace liong
