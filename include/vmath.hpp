// Vector math utilities.
#pragma once
#include <cstdint>
#include <cmath>
#include <ostream>

namespace liong {
namespace vmath {
struct uint2 {
  uint32_t x, y;
  uint2() = default;
  uint2(const uint2&) = default;
  uint2(uint2&&) = default;
  uint2& operator=(const uint2&) = default;
  uint2& operator=(uint2&&) = default;
  inline uint2(uint32_t x, uint32_t y) : x(x), y(y) {}
  uint2(const struct int2& a);
  uint2(const struct float2& a);
  uint2(const struct bool2& a);
  friend std::ostream& operator<<(std::ostream& out, const uint2& a) { return out << "(" << a.x << ", " << a.y << ")"; }
};
inline uint2 operator+(const uint2& a, const uint2& b) { return uint2 { a.x + b.x, a.y + b.y }; }
inline uint2 operator-(const uint2& a, const uint2& b) { return uint2 { a.x - b.x, a.y - b.y }; }
inline uint2 operator*(const uint2& a, const uint2& b) { return uint2 { a.x * b.x, a.y * b.y }; }
inline uint2 operator/(const uint2& a, const uint2& b) { return uint2 { a.x / b.x, a.y / b.y }; }
inline uint2 min(const uint2& a, const uint2& b) { return uint2 { std::min(a.x, b.x), std::min(a.y, b.y) }; }
inline uint2 max(const uint2& a, const uint2& b) { return uint2 { std::max(a.x, b.x), std::max(a.y, b.y) }; }
inline uint2 operator%(const uint2& a, const uint2& b) { return uint2 { a.x % b.x, a.y % b.y }; }
inline uint2 operator&(const uint2& a, const uint2& b) { return uint2 { a.x & b.x, a.y & b.y }; }
inline uint2 operator|(const uint2& a, const uint2& b) { return uint2 { a.x | b.x, a.y | b.y }; }
inline uint2 operator^(const uint2& a, const uint2& b) { return uint2 { a.x ^ b.x, a.y ^ b.y }; }
inline uint2 operator~(const uint2& a) { return uint2 { ~a.x, ~a.y }; }
struct uint3 {
  uint32_t x, y, z;
  uint3() = default;
  uint3(const uint3&) = default;
  uint3(uint3&&) = default;
  uint3& operator=(const uint3&) = default;
  uint3& operator=(uint3&&) = default;
  inline uint3(uint32_t x, uint32_t y, uint32_t z) : x(x), y(y), z(z) {}
  inline uint3(const struct int3& a);
  inline uint3(const struct float3& a);
  inline uint3(const struct bool3& a);
  friend std::ostream& operator<<(std::ostream& out, const uint3& a) { return out << "(" << a.x << ", " << a.y << ", " << a.z << ")"; }
};
inline uint3 operator+(const uint3& a, const uint3& b) { return uint3 { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline uint3 operator-(const uint3& a, const uint3& b) { return uint3 { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline uint3 operator*(const uint3& a, const uint3& b) { return uint3 { a.x * b.x, a.y * b.y, a.z * b.z }; }
inline uint3 operator/(const uint3& a, const uint3& b) { return uint3 { a.x / b.x, a.y / b.y, a.z / b.z }; }
inline uint3 min(const uint3& a, const uint3& b) { return uint3 { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) }; }
inline uint3 max(const uint3& a, const uint3& b) { return uint3 { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) }; }
inline uint3 operator%(const uint3& a, const uint3& b) { return uint3 { a.x % b.x, a.y % b.y, a.z % b.z }; }
inline uint3 operator&(const uint3& a, const uint3& b) { return uint3 { a.x & b.x, a.y & b.y, a.z & b.z }; }
inline uint3 operator|(const uint3& a, const uint3& b) { return uint3 { a.x | b.x, a.y | b.y, a.z | b.z }; }
inline uint3 operator^(const uint3& a, const uint3& b) { return uint3 { a.x ^ b.x, a.y ^ b.y, a.z ^ b.z }; }
inline uint3 operator~(const uint3& a) { return uint3 { ~a.x, ~a.y, ~a.z }; }
struct uint4 {
  uint32_t x, y, z, w;
  uint4() = default;
  uint4(const uint4&) = default;
  uint4(uint4&&) = default;
  uint4& operator=(const uint4&) = default;
  uint4& operator=(uint4&&) = default;
  inline uint4(uint32_t x, uint32_t y, uint32_t z, uint32_t w) : x(x), y(y), z(z), w(w) {}
  inline uint4(const struct int4& a);
  inline uint4(const struct float4& a);
  inline uint4(const struct bool4& a);
  friend std::ostream& operator<<(std::ostream& out, const uint4& a) { return out << "(" << a.x << ", " << a.y << ", " << a.z << ", " << a.w << ")"; }
};
inline uint4 operator+(const uint4& a, const uint4& b) { return uint4 { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
inline uint4 operator-(const uint4& a, const uint4& b) { return uint4 { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }
inline uint4 operator*(const uint4& a, const uint4& b) { return uint4 { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w }; }
inline uint4 operator/(const uint4& a, const uint4& b) { return uint4 { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w }; }
inline uint4 min(const uint4& a, const uint4& b) { return uint4 { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z), std::min(a.w, b.w) }; }
inline uint4 max(const uint4& a, const uint4& b) { return uint4 { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z), std::max(a.w, b.w) }; }
inline uint4 operator%(const uint4& a, const uint4& b) { return uint4 { a.x % b.x, a.y % b.y, a.z % b.z, a.w % b.w }; }
inline uint4 operator&(const uint4& a, const uint4& b) { return uint4 { a.x & b.x, a.y & b.y, a.z & b.z, a.w & b.w }; }
inline uint4 operator|(const uint4& a, const uint4& b) { return uint4 { a.x | b.x, a.y | b.y, a.z | b.z, a.w | b.w }; }
inline uint4 operator^(const uint4& a, const uint4& b) { return uint4 { a.x ^ b.x, a.y ^ b.y, a.z ^ b.z, a.w ^ b.w }; }
inline uint4 operator~(const uint4& a) { return uint4 { ~a.x, ~a.y, ~a.z, ~a.w }; }
struct int2 {
  int32_t x, y;
  int2() = default;
  int2(const int2&) = default;
  int2(int2&&) = default;
  int2& operator=(const int2&) = default;
  int2& operator=(int2&&) = default;
  inline int2(int32_t x, int32_t y) : x(x), y(y) {}
  int2(const struct uint2& a);
  int2(const struct float2& a);
  int2(const struct bool2& a);
  friend std::ostream& operator<<(std::ostream& out, const int2& a) { return out << "(" << a.x << ", " << a.y << ")"; }
};
inline int2 operator+(const int2& a, const int2& b) { return int2 { a.x + b.x, a.y + b.y }; }
inline int2 operator-(const int2& a, const int2& b) { return int2 { a.x - b.x, a.y - b.y }; }
inline int2 operator*(const int2& a, const int2& b) { return int2 { a.x * b.x, a.y * b.y }; }
inline int2 operator/(const int2& a, const int2& b) { return int2 { a.x / b.x, a.y / b.y }; }
inline int2 operator-(const int2& a) { return int2 { -a.x, -a.y }; }
inline int2 min(const int2& a, const int2& b) { return int2 { std::min(a.x, b.x), std::min(a.y, b.y) }; }
inline int2 max(const int2& a, const int2& b) { return int2 { std::max(a.x, b.x), std::max(a.y, b.y) }; }
inline int2 operator%(const int2& a, const int2& b) { return int2 { a.x % b.x, a.y % b.y }; }
inline int2 operator&(const int2& a, const int2& b) { return int2 { a.x & b.x, a.y & b.y }; }
inline int2 operator|(const int2& a, const int2& b) { return int2 { a.x | b.x, a.y | b.y }; }
inline int2 operator^(const int2& a, const int2& b) { return int2 { a.x ^ b.x, a.y ^ b.y }; }
inline int2 operator~(const int2& a) { return int2 { ~a.x, ~a.y }; }
struct int3 {
  int32_t x, y, z;
  int3() = default;
  int3(const int3&) = default;
  int3(int3&&) = default;
  int3& operator=(const int3&) = default;
  int3& operator=(int3&&) = default;
  inline int3(int32_t x, int32_t y, int32_t z) : x(x), y(y), z(z) {}
  inline int3(const struct uint3& a);
  inline int3(const struct float3& a);
  inline int3(const struct bool3& a);
  friend std::ostream& operator<<(std::ostream& out, const int3& a) { return out << "(" << a.x << ", " << a.y << ", " << a.z << ")"; }
};
inline int3 operator+(const int3& a, const int3& b) { return int3 { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline int3 operator-(const int3& a, const int3& b) { return int3 { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline int3 operator*(const int3& a, const int3& b) { return int3 { a.x * b.x, a.y * b.y, a.z * b.z }; }
inline int3 operator/(const int3& a, const int3& b) { return int3 { a.x / b.x, a.y / b.y, a.z / b.z }; }
inline int3 operator-(const int3& a) { return int3 { -a.x, -a.y, -a.z }; }
inline int3 min(const int3& a, const int3& b) { return int3 { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) }; }
inline int3 max(const int3& a, const int3& b) { return int3 { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) }; }
inline int3 operator%(const int3& a, const int3& b) { return int3 { a.x % b.x, a.y % b.y, a.z % b.z }; }
inline int3 operator&(const int3& a, const int3& b) { return int3 { a.x & b.x, a.y & b.y, a.z & b.z }; }
inline int3 operator|(const int3& a, const int3& b) { return int3 { a.x | b.x, a.y | b.y, a.z | b.z }; }
inline int3 operator^(const int3& a, const int3& b) { return int3 { a.x ^ b.x, a.y ^ b.y, a.z ^ b.z }; }
inline int3 operator~(const int3& a) { return int3 { ~a.x, ~a.y, ~a.z }; }
struct int4 {
  int32_t x, y, z, w;
  int4() = default;
  int4(const int4&) = default;
  int4(int4&&) = default;
  int4& operator=(const int4&) = default;
  int4& operator=(int4&&) = default;
  inline int4(int32_t x, int32_t y, int32_t z, int32_t w) : x(x), y(y), z(z), w(w) {}
  inline int4(const struct uint4& a);
  inline int4(const struct float4& a);
  inline int4(const struct bool4& a);
  friend std::ostream& operator<<(std::ostream& out, const int4& a) { return out << "(" << a.x << ", " << a.y << ", " << a.z << ", " << a.w << ")"; }
};
inline int4 operator+(const int4& a, const int4& b) { return int4 { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
inline int4 operator-(const int4& a, const int4& b) { return int4 { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }
inline int4 operator*(const int4& a, const int4& b) { return int4 { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w }; }
inline int4 operator/(const int4& a, const int4& b) { return int4 { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w }; }
inline int4 operator-(const int4& a) { return int4 { -a.x, -a.y, -a.z, -a.w }; }
inline int4 min(const int4& a, const int4& b) { return int4 { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z), std::min(a.w, b.w) }; }
inline int4 max(const int4& a, const int4& b) { return int4 { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z), std::max(a.w, b.w) }; }
inline int4 operator%(const int4& a, const int4& b) { return int4 { a.x % b.x, a.y % b.y, a.z % b.z, a.w % b.w }; }
inline int4 operator&(const int4& a, const int4& b) { return int4 { a.x & b.x, a.y & b.y, a.z & b.z, a.w & b.w }; }
inline int4 operator|(const int4& a, const int4& b) { return int4 { a.x | b.x, a.y | b.y, a.z | b.z, a.w | b.w }; }
inline int4 operator^(const int4& a, const int4& b) { return int4 { a.x ^ b.x, a.y ^ b.y, a.z ^ b.z, a.w ^ b.w }; }
inline int4 operator~(const int4& a) { return int4 { ~a.x, ~a.y, ~a.z, ~a.w }; }
struct float2 {
  float x, y;
  float2() = default;
  float2(const float2&) = default;
  float2(float2&&) = default;
  float2& operator=(const float2&) = default;
  float2& operator=(float2&&) = default;
  inline float2(float x, float y) : x(x), y(y) {}
  float2(const struct uint2& a);
  float2(const struct int2& a);
  float2(const struct bool2& a);
  friend std::ostream& operator<<(std::ostream& out, const float2& a) { return out << "(" << a.x << ", " << a.y << ")"; }
};
inline float2 operator+(const float2& a, const float2& b) { return float2 { a.x + b.x, a.y + b.y }; }
inline float2 operator-(const float2& a, const float2& b) { return float2 { a.x - b.x, a.y - b.y }; }
inline float2 operator*(const float2& a, const float2& b) { return float2 { a.x * b.x, a.y * b.y }; }
inline float2 operator/(const float2& a, const float2& b) { return float2 { a.x / b.x, a.y / b.y }; }
inline float2 operator-(const float2& a) { return float2 { -a.x, -a.y }; }
inline float2 min(const float2& a, const float2& b) { return float2 { std::min(a.x, b.x), std::min(a.y, b.y) }; }
inline float2 max(const float2& a, const float2& b) { return float2 { std::max(a.x, b.x), std::max(a.y, b.y) }; }
inline float2 atan2(const float2& a, const float2& b) { return float2 { std::atan2f(a.x, b.x), std::atan2f(a.y, b.y) }; }
inline float2 abs(const float2& a, const float2& b) { return float2 { std::fabs(a.x), std::fabs(a.y) }; }
inline float2 floor(const float2& a, const float2& b) { return float2 { std::floorf(a.x), std::floorf(a.y) }; }
inline float2 ceil(const float2& a, const float2& b) { return float2 { std::ceilf(a.x), std::ceilf(a.y) }; }
inline float2 round(const float2& a, const float2& b) { return float2 { std::roundf(a.x), std::roundf(a.y) }; }
inline float2 sqrt(const float2& a, const float2& b) { return float2 { std::sqrt(a.x), std::sqrt(a.y) }; }
inline float2 trunc(const float2& a, const float2& b) { return float2 { std::truncf(a.x), std::truncf(a.y) }; }
inline float2 sin(const float2& a, const float2& b) { return float2 { std::sinf(a.x), std::sinf(a.y) }; }
inline float2 cos(const float2& a, const float2& b) { return float2 { std::cosf(a.x), std::cosf(a.y) }; }
inline float2 tan(const float2& a, const float2& b) { return float2 { std::tanf(a.x), std::tanf(a.y) }; }
inline float2 sinh(const float2& a, const float2& b) { return float2 { std::sinhf(a.x), std::sinhf(a.y) }; }
inline float2 cosh(const float2& a, const float2& b) { return float2 { std::coshf(a.x), std::coshf(a.y) }; }
inline float2 tanh(const float2& a, const float2& b) { return float2 { std::tanhf(a.x), std::tanhf(a.y) }; }
inline float2 asin(const float2& a, const float2& b) { return float2 { std::asinf(a.x), std::asinf(a.y) }; }
inline float2 acos(const float2& a, const float2& b) { return float2 { std::acosf(a.x), std::acosf(a.y) }; }
inline float2 atan(const float2& a, const float2& b) { return float2 { std::atanf(a.x), std::atanf(a.y) }; }
inline float2 asinh(const float2& a, const float2& b) { return float2 { std::asinhf(a.x), std::asinhf(a.y) }; }
inline float2 acosh(const float2& a, const float2& b) { return float2 { std::acoshf(a.x), std::acoshf(a.y) }; }
inline float2 atanh(const float2& a, const float2& b) { return float2 { std::atanhf(a.x), std::atanhf(a.y) }; }
struct float3 {
  float x, y, z;
  float3() = default;
  float3(const float3&) = default;
  float3(float3&&) = default;
  float3& operator=(const float3&) = default;
  float3& operator=(float3&&) = default;
  inline float3(float x, float y, float z) : x(x), y(y), z(z) {}
  inline float3(const struct uint3& a);
  inline float3(const struct int3& a);
  inline float3(const struct bool3& a);
  friend std::ostream& operator<<(std::ostream& out, const float3& a) { return out << "(" << a.x << ", " << a.y << ", " << a.z << ")"; }
};
inline float3 operator+(const float3& a, const float3& b) { return float3 { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline float3 operator-(const float3& a, const float3& b) { return float3 { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline float3 operator*(const float3& a, const float3& b) { return float3 { a.x * b.x, a.y * b.y, a.z * b.z }; }
inline float3 operator/(const float3& a, const float3& b) { return float3 { a.x / b.x, a.y / b.y, a.z / b.z }; }
inline float3 operator-(const float3& a) { return float3 { -a.x, -a.y, -a.z }; }
inline float3 min(const float3& a, const float3& b) { return float3 { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) }; }
inline float3 max(const float3& a, const float3& b) { return float3 { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) }; }
inline float3 atan2(const float3& a, const float3& b) { return float3 { std::atan2f(a.x, b.x), std::atan2f(a.y, b.y), std::atan2f(a.z, b.z) }; }
inline float3 abs(const float3& a, const float3& b) { return float3 { std::fabs(a.x), std::fabs(a.y), std::fabs(a.z) }; }
inline float3 floor(const float3& a, const float3& b) { return float3 { std::floorf(a.x), std::floorf(a.y), std::floorf(a.z) }; }
inline float3 ceil(const float3& a, const float3& b) { return float3 { std::ceilf(a.x), std::ceilf(a.y), std::ceilf(a.z) }; }
inline float3 round(const float3& a, const float3& b) { return float3 { std::roundf(a.x), std::roundf(a.y), std::roundf(a.z) }; }
inline float3 sqrt(const float3& a, const float3& b) { return float3 { std::sqrt(a.x), std::sqrt(a.y), std::sqrt(a.z) }; }
inline float3 trunc(const float3& a, const float3& b) { return float3 { std::truncf(a.x), std::truncf(a.y), std::truncf(a.z) }; }
inline float3 sin(const float3& a, const float3& b) { return float3 { std::sinf(a.x), std::sinf(a.y), std::sinf(a.z) }; }
inline float3 cos(const float3& a, const float3& b) { return float3 { std::cosf(a.x), std::cosf(a.y), std::cosf(a.z) }; }
inline float3 tan(const float3& a, const float3& b) { return float3 { std::tanf(a.x), std::tanf(a.y), std::tanf(a.z) }; }
inline float3 sinh(const float3& a, const float3& b) { return float3 { std::sinhf(a.x), std::sinhf(a.y), std::sinhf(a.z) }; }
inline float3 cosh(const float3& a, const float3& b) { return float3 { std::coshf(a.x), std::coshf(a.y), std::coshf(a.z) }; }
inline float3 tanh(const float3& a, const float3& b) { return float3 { std::tanhf(a.x), std::tanhf(a.y), std::tanhf(a.z) }; }
inline float3 asin(const float3& a, const float3& b) { return float3 { std::asinf(a.x), std::asinf(a.y), std::asinf(a.z) }; }
inline float3 acos(const float3& a, const float3& b) { return float3 { std::acosf(a.x), std::acosf(a.y), std::acosf(a.z) }; }
inline float3 atan(const float3& a, const float3& b) { return float3 { std::atanf(a.x), std::atanf(a.y), std::atanf(a.z) }; }
inline float3 asinh(const float3& a, const float3& b) { return float3 { std::asinhf(a.x), std::asinhf(a.y), std::asinhf(a.z) }; }
inline float3 acosh(const float3& a, const float3& b) { return float3 { std::acoshf(a.x), std::acoshf(a.y), std::acoshf(a.z) }; }
inline float3 atanh(const float3& a, const float3& b) { return float3 { std::atanhf(a.x), std::atanhf(a.y), std::atanhf(a.z) }; }
struct float4 {
  float x, y, z, w;
  float4() = default;
  float4(const float4&) = default;
  float4(float4&&) = default;
  float4& operator=(const float4&) = default;
  float4& operator=(float4&&) = default;
  inline float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
  inline float4(const struct uint4& a);
  inline float4(const struct int4& a);
  inline float4(const struct bool4& a);
  friend std::ostream& operator<<(std::ostream& out, const float4& a) { return out << "(" << a.x << ", " << a.y << ", " << a.z << ", " << a.w << ")"; }
};
inline float4 operator+(const float4& a, const float4& b) { return float4 { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
inline float4 operator-(const float4& a, const float4& b) { return float4 { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }
inline float4 operator*(const float4& a, const float4& b) { return float4 { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w }; }
inline float4 operator/(const float4& a, const float4& b) { return float4 { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w }; }
inline float4 operator-(const float4& a) { return float4 { -a.x, -a.y, -a.z, -a.w }; }
inline float4 min(const float4& a, const float4& b) { return float4 { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z), std::min(a.w, b.w) }; }
inline float4 max(const float4& a, const float4& b) { return float4 { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z), std::max(a.w, b.w) }; }
inline float4 atan2(const float4& a, const float4& b) { return float4 { std::atan2f(a.x, b.x), std::atan2f(a.y, b.y), std::atan2f(a.z, b.z), std::atan2f(a.w, b.w) }; }
inline float4 abs(const float4& a, const float4& b) { return float4 { std::fabs(a.x), std::fabs(a.y), std::fabs(a.z), std::fabs(a.w) }; }
inline float4 floor(const float4& a, const float4& b) { return float4 { std::floorf(a.x), std::floorf(a.y), std::floorf(a.z), std::floorf(a.w) }; }
inline float4 ceil(const float4& a, const float4& b) { return float4 { std::ceilf(a.x), std::ceilf(a.y), std::ceilf(a.z), std::ceilf(a.w) }; }
inline float4 round(const float4& a, const float4& b) { return float4 { std::roundf(a.x), std::roundf(a.y), std::roundf(a.z), std::roundf(a.w) }; }
inline float4 sqrt(const float4& a, const float4& b) { return float4 { std::sqrt(a.x), std::sqrt(a.y), std::sqrt(a.z), std::sqrt(a.w) }; }
inline float4 trunc(const float4& a, const float4& b) { return float4 { std::truncf(a.x), std::truncf(a.y), std::truncf(a.z), std::truncf(a.w) }; }
inline float4 sin(const float4& a, const float4& b) { return float4 { std::sinf(a.x), std::sinf(a.y), std::sinf(a.z), std::sinf(a.w) }; }
inline float4 cos(const float4& a, const float4& b) { return float4 { std::cosf(a.x), std::cosf(a.y), std::cosf(a.z), std::cosf(a.w) }; }
inline float4 tan(const float4& a, const float4& b) { return float4 { std::tanf(a.x), std::tanf(a.y), std::tanf(a.z), std::tanf(a.w) }; }
inline float4 sinh(const float4& a, const float4& b) { return float4 { std::sinhf(a.x), std::sinhf(a.y), std::sinhf(a.z), std::sinhf(a.w) }; }
inline float4 cosh(const float4& a, const float4& b) { return float4 { std::coshf(a.x), std::coshf(a.y), std::coshf(a.z), std::coshf(a.w) }; }
inline float4 tanh(const float4& a, const float4& b) { return float4 { std::tanhf(a.x), std::tanhf(a.y), std::tanhf(a.z), std::tanhf(a.w) }; }
inline float4 asin(const float4& a, const float4& b) { return float4 { std::asinf(a.x), std::asinf(a.y), std::asinf(a.z), std::asinf(a.w) }; }
inline float4 acos(const float4& a, const float4& b) { return float4 { std::acosf(a.x), std::acosf(a.y), std::acosf(a.z), std::acosf(a.w) }; }
inline float4 atan(const float4& a, const float4& b) { return float4 { std::atanf(a.x), std::atanf(a.y), std::atanf(a.z), std::atanf(a.w) }; }
inline float4 asinh(const float4& a, const float4& b) { return float4 { std::asinhf(a.x), std::asinhf(a.y), std::asinhf(a.z), std::asinhf(a.w) }; }
inline float4 acosh(const float4& a, const float4& b) { return float4 { std::acoshf(a.x), std::acoshf(a.y), std::acoshf(a.z), std::acoshf(a.w) }; }
inline float4 atanh(const float4& a, const float4& b) { return float4 { std::atanhf(a.x), std::atanhf(a.y), std::atanhf(a.z), std::atanhf(a.w) }; }
struct bool2 {
  bool x, y;
  bool2() = default;
  bool2(const bool2&) = default;
  bool2(bool2&&) = default;
  bool2& operator=(const bool2&) = default;
  bool2& operator=(bool2&&) = default;
  inline bool2(bool x, bool y) : x(x), y(y) {}
  bool2(const struct uint2& a);
  bool2(const struct int2& a);
  bool2(const struct float2& a);
  friend std::ostream& operator<<(std::ostream& out, const bool2& a) { return out << "(" << a.x << ", " << a.y << ")"; }
};
inline bool2 operator&&(const bool2& a, const bool2& b) { return bool2 { a.x && b.x, a.y && b.y }; }
inline bool2 operator||(const bool2& a, const bool2& b) { return bool2 { a.x || b.x, a.y || b.y }; }
inline bool2 operator!(const bool2& a) { return bool2 { !a.x, !a.y }; }
struct bool3 {
  bool x, y, z;
  bool3() = default;
  bool3(const bool3&) = default;
  bool3(bool3&&) = default;
  bool3& operator=(const bool3&) = default;
  bool3& operator=(bool3&&) = default;
  inline bool3(bool x, bool y, bool z) : x(x), y(y), z(z) {}
  inline bool3(const struct uint3& a);
  inline bool3(const struct int3& a);
  inline bool3(const struct float3& a);
  friend std::ostream& operator<<(std::ostream& out, const bool3& a) { return out << "(" << a.x << ", " << a.y << ", " << a.z << ")"; }
};
inline bool3 operator&&(const bool3& a, const bool3& b) { return bool3 { a.x && b.x, a.y && b.y, a.z && b.z }; }
inline bool3 operator||(const bool3& a, const bool3& b) { return bool3 { a.x || b.x, a.y || b.y, a.z || b.z }; }
inline bool3 operator!(const bool3& a) { return bool3 { !a.x, !a.y, !a.z }; }
struct bool4 {
  bool x, y, z, w;
  bool4() = default;
  bool4(const bool4&) = default;
  bool4(bool4&&) = default;
  bool4& operator=(const bool4&) = default;
  bool4& operator=(bool4&&) = default;
  inline bool4(bool x, bool y, bool z, bool w) : x(x), y(y), z(z), w(w) {}
  inline bool4(const struct uint4& a);
  inline bool4(const struct int4& a);
  inline bool4(const struct float4& a);
  friend std::ostream& operator<<(std::ostream& out, const bool4& a) { return out << "(" << a.x << ", " << a.y << ", " << a.z << ", " << a.w << ")"; }
};
inline bool4 operator&&(const bool4& a, const bool4& b) { return bool4 { a.x && b.x, a.y && b.y, a.z && b.z, a.w && b.w }; }
inline bool4 operator||(const bool4& a, const bool4& b) { return bool4 { a.x || b.x, a.y || b.y, a.z || b.z, a.w || b.w }; }
inline bool4 operator!(const bool4& a) { return bool4 { !a.x, !a.y, !a.z, !a.w }; }
  inline uint2::uint2(const int2& a) : x((uint32_t)a.x), y((uint32_t)a.y) {}
  inline uint2::uint2(const float2& a) : x((uint32_t)a.x), y((uint32_t)a.y) {}
  inline uint2::uint2(const bool2& a) : x((uint32_t)a.x), y((uint32_t)a.y) {}
  inline uint3::uint3(const int3& a) : x((uint32_t)a.x), y((uint32_t)a.y), z((uint32_t)a.z) {}
  inline uint3::uint3(const float3& a) : x((uint32_t)a.x), y((uint32_t)a.y), z((uint32_t)a.z) {}
  inline uint3::uint3(const bool3& a) : x((uint32_t)a.x), y((uint32_t)a.y), z((uint32_t)a.z) {}
  inline uint4::uint4(const int4& a) : x((uint32_t)a.x), y((uint32_t)a.y), z((uint32_t)a.z), w((uint32_t)a.w) {}
  inline uint4::uint4(const float4& a) : x((uint32_t)a.x), y((uint32_t)a.y), z((uint32_t)a.z), w((uint32_t)a.w) {}
  inline uint4::uint4(const bool4& a) : x((uint32_t)a.x), y((uint32_t)a.y), z((uint32_t)a.z), w((uint32_t)a.w) {}
  inline int2::int2(const uint2& a) : x((int32_t)a.x), y((int32_t)a.y) {}
  inline int2::int2(const float2& a) : x((int32_t)a.x), y((int32_t)a.y) {}
  inline int2::int2(const bool2& a) : x((int32_t)a.x), y((int32_t)a.y) {}
  inline int3::int3(const uint3& a) : x((int32_t)a.x), y((int32_t)a.y), z((int32_t)a.z) {}
  inline int3::int3(const float3& a) : x((int32_t)a.x), y((int32_t)a.y), z((int32_t)a.z) {}
  inline int3::int3(const bool3& a) : x((int32_t)a.x), y((int32_t)a.y), z((int32_t)a.z) {}
  inline int4::int4(const uint4& a) : x((int32_t)a.x), y((int32_t)a.y), z((int32_t)a.z), w((int32_t)a.w) {}
  inline int4::int4(const float4& a) : x((int32_t)a.x), y((int32_t)a.y), z((int32_t)a.z), w((int32_t)a.w) {}
  inline int4::int4(const bool4& a) : x((int32_t)a.x), y((int32_t)a.y), z((int32_t)a.z), w((int32_t)a.w) {}
  inline float2::float2(const uint2& a) : x((float)a.x), y((float)a.y) {}
  inline float2::float2(const int2& a) : x((float)a.x), y((float)a.y) {}
  inline float2::float2(const bool2& a) : x((float)a.x), y((float)a.y) {}
  inline float3::float3(const uint3& a) : x((float)a.x), y((float)a.y), z((float)a.z) {}
  inline float3::float3(const int3& a) : x((float)a.x), y((float)a.y), z((float)a.z) {}
  inline float3::float3(const bool3& a) : x((float)a.x), y((float)a.y), z((float)a.z) {}
  inline float4::float4(const uint4& a) : x((float)a.x), y((float)a.y), z((float)a.z), w((float)a.w) {}
  inline float4::float4(const int4& a) : x((float)a.x), y((float)a.y), z((float)a.z), w((float)a.w) {}
  inline float4::float4(const bool4& a) : x((float)a.x), y((float)a.y), z((float)a.z), w((float)a.w) {}
  inline bool2::bool2(const uint2& a) : x((bool)a.x), y((bool)a.y) {}
  inline bool2::bool2(const int2& a) : x((bool)a.x), y((bool)a.y) {}
  inline bool2::bool2(const float2& a) : x((bool)a.x), y((bool)a.y) {}
  inline bool3::bool3(const uint3& a) : x((bool)a.x), y((bool)a.y), z((bool)a.z) {}
  inline bool3::bool3(const int3& a) : x((bool)a.x), y((bool)a.y), z((bool)a.z) {}
  inline bool3::bool3(const float3& a) : x((bool)a.x), y((bool)a.y), z((bool)a.z) {}
  inline bool4::bool4(const uint4& a) : x((bool)a.x), y((bool)a.y), z((bool)a.z), w((bool)a.w) {}
  inline bool4::bool4(const int4& a) : x((bool)a.x), y((bool)a.y), z((bool)a.z), w((bool)a.w) {}
  inline bool4::bool4(const float4& a) : x((bool)a.x), y((bool)a.y), z((bool)a.z), w((bool)a.w) {}
} // namespace vmath
} // namespace liong
