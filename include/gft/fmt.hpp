// Color and depth-stencil pixel format specification.
// @PENGUINLIONG
#pragma once
#include <cstdint>
#include "gft/assert.hpp"
#include "glm/glm.hpp"

namespace liong {

namespace fmt {

enum Format {
  L_FORMAT_UNDEFINED,
  L_FORMAT_R8G8B8A8_UNORM,
  L_FORMAT_B8G8R8A8_UNORM,
  L_FORMAT_B10G11R11_UFLOAT_PACK32,
  L_FORMAT_R16G16B16A16_SFLOAT,
  L_FORMAT_R32_SFLOAT,
  L_FORMAT_R32G32_SFLOAT,
  L_FORMAT_R32G32B32A32_SFLOAT,
};

enum DepthFormat {
  L_DEPTH_FORMAT_UNDEFINED,
  L_DEPTH_FORMAT_D16_UNORM,
  L_DEPTH_FORMAT_D32_SFLOAT,
};

constexpr size_t get_fmt_size(Format fmt) {
  return fmt == L_FORMAT_R8G8B8A8_UNORM            ? 4
         : fmt == L_FORMAT_B8G8R8A8_UNORM          ? 4
         : fmt == L_FORMAT_B10G11R11_UFLOAT_PACK32 ? 4
         : fmt == L_FORMAT_R16G16B16A16_SFLOAT     ? 8
         : fmt == L_FORMAT_R32_SFLOAT              ? 4
         : fmt == L_FORMAT_R32G32_SFLOAT           ? 8
         : fmt == L_FORMAT_R32G32B32A32_SFLOAT     ? 16
                                                   : 0;
}
constexpr size_t get_fmt_depth_nbit(DepthFormat fmt) {
  return fmt == L_DEPTH_FORMAT_D16_UNORM    ? 16
         : fmt == L_DEPTH_FORMAT_D32_SFLOAT ? 32
                                            : 0;
}
constexpr size_t get_fmt_stencil_nbit(DepthFormat fmt) {
  return 0;
}

template<Format Fmt>
struct FormatCodec {};

template<>
struct FormatCodec<L_FORMAT_R8G8B8A8_UNORM> {
  static void encode(const glm::vec4* src, const void* dst, uint32_t npx) {
    for (uint32_t i = 0; i < npx; ++i) {
      glm::vec4 vec = src[i];
      ((uint32_t*)dst)[i] =
        (((uint32_t)(std::min(std::max(vec.x, 0.0f), 1.0f) * 255.0f)) << 0) |
        (((uint32_t)(std::min(std::max(vec.y, 0.0f), 1.0f) * 255.0f)) << 8) |
        (((uint32_t)(std::min(std::max(vec.z, 0.0f), 1.0f) * 255.0f)) << 16) |
        (((uint32_t)(std::min(std::max(vec.w, 0.0f), 1.0f) * 255.0f)) << 24);
    }
  }
  static void decode(const void* src, glm::vec4* dst, uint32_t npx) {
    for (uint32_t i = 0; i < npx; ++i) {
      uint32_t pack = ((const uint32_t*)src)[i];
      dst[i] = glm::vec4 {
        (float)((pack >> 0) & 0xFF) / 255.0f,
        (float)((pack >> 8) & 0xFF) / 255.0f,
        (float)((pack >> 16) & 0xFF) / 255.0f,
        (float)((pack >> 24) & 0xFF) / 255.0f,
      };
    }
  }
};

// NOTE: It doesn't deal with NaN and infinities correctly. Just doesn't care.
template<>
struct FormatCodec<L_FORMAT_R16G16B16A16_SFLOAT> {
  static void encode(const glm::vec4* src, const void* dst, uint32_t npx) {
    auto float2half = [](uint32_t x) {
      uint32_t sign_bit = x >> 31;
      uint32_t exponent = (x >> 23) & 255;
      if (exponent == 255 || exponent < (127 - 15) || exponent >= 127 + 15) {
        return 0;
      }
      uint32_t fraction = x & 8388607;
      unimplemented();
    };
    for (uint32_t i = 0; i < npx; ++i) {
      uint16_t f0 = float2half(*(const uint32_t*)(&src[i].x));
      uint16_t f1 = float2half(*(const uint32_t*)(&src[i].y));
      uint16_t f2 = float2half(*(const uint32_t*)(&src[i].z));
      uint16_t f3 = float2half(*(const uint32_t*)(&src[i].w));
      ((uint16_t*)dst)[i * 4 + 0] = f0;
      ((uint16_t*)dst)[i * 4 + 1] = f1;
      ((uint16_t*)dst)[i * 4 + 2] = f2;
      ((uint16_t*)dst)[i * 4 + 3] = f3;
    }
  }
  static void decode(const void* src, glm::vec4* dst, uint32_t npx) {
    auto half2float = [](uint16_t x) {
      uint32_t sign_bit = x >> 15;
      uint32_t exponent = (x >> 10) & 31;
      uint32_t mantissa = x & 1023;
      if (exponent == 31) {
        return (uint32_t)0;
      } // NaN or infinity.
      exponent = exponent - 15 + 127;
      mantissa <<= 23 - 10;
      uint32_t out = (sign_bit << 31) | (exponent << 23) | mantissa;
      return out;
    };
    for (uint32_t i = 0; i < npx; ++i) {
      uint32_t f0 = half2float(((const uint16_t*)src)[i * 4 + 0]);
      uint32_t f1 = half2float(((const uint16_t*)src)[i * 4 + 1]);
      uint32_t f2 = half2float(((const uint16_t*)src)[i * 4 + 2]);
      uint32_t f3 = half2float(((const uint16_t*)src)[i * 4 + 3]);
      dst[i] = glm::vec4 {
        *(const float*)(&f0),
        *(const float*)(&f1),
        *(const float*)(&f2),
        *(const float*)(&f3),
      };
    }
  }
};
template<>
struct FormatCodec<L_FORMAT_R32_SFLOAT> {
  static void encode(const glm::vec4* src, void* dst, uint32_t npx) {
    for (uint32_t i = 0; i < npx; ++i) {
      ((float*)dst)[i] = src[i].x;
    }
  }
  static void decode(const void* src, glm::vec4* dst, uint32_t npx) {
    for (uint32_t i = 0; i < npx; ++i) {
      glm::vec4 vec {
        ((const float*)src)[i],
        0.0f,
        0.0f,
        0.0f,
      };
      dst[i] = std::move(vec);
    }
  }
};

template<>
struct FormatCodec<L_FORMAT_R32G32_SFLOAT> {
  static void encode(const glm::vec4* src, void* dst, uint32_t npx) {
    for (uint32_t i = 0; i < npx; ++i) {
      ((float*)dst)[i * 2 + 0] = src[i].x;
      ((float*)dst)[i * 2 + 1] = src[i].y;
    }
  }
  static void decode(const void* src, glm::vec4* dst, uint32_t npx) {
    for (uint32_t i = 0; i < npx; ++i) {
      glm::vec4 vec {
        ((const float*)src)[i * 2 + 0],
        ((const float*)src)[i * 2 + 1],
        0.0f,
        0.0f,
      };
      dst[i] = std::move(vec);
    }
  }
};

template<>
struct FormatCodec<L_FORMAT_R32G32B32A32_SFLOAT> {
  static void encode(const glm::vec4* src, void* dst, uint32_t npx) {
    for (uint32_t i = 0; i < npx; ++i) {
      ((float*)dst)[i * 4 + 0] = src[i].x;
      ((float*)dst)[i * 4 + 1] = src[i].y;
      ((float*)dst)[i * 4 + 2] = src[i].z;
      ((float*)dst)[i * 4 + 3] = src[i].w;
    }
  }
  static void decode(const void* src, glm::vec4* dst, uint32_t npx) {
    for (uint32_t i = 0; i < npx; ++i) {
      glm::vec4 vec {
        ((const float*)src)[i * 4 + 0],
        ((const float*)src)[i * 4 + 1],
        ((const float*)src)[i * 4 + 2],
        ((const float*)src)[i * 4 + 3],
      };
      dst[i] = std::move(vec);
    }
  }
};

enum ColorSpace {
  L_COLOR_SPACE_LINEAR,
  L_COLOR_SPACE_SRGB,
};

} // namespace fmt

} // namespace liong
