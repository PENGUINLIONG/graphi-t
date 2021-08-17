// # Hardware abstraction layer
// @PENGUINLIONG
//
// This file defines all APIs to be implemented by each platform and provides
// interfacing structures to the most common extent.
//
// Before implementing a HAL, please include `common.hpp` at the beginning of
// your header.

// Don't use `#pragma once` here.
#pragma once
#include <cmath>
#include <array>
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include "assert.hpp"

#ifndef HAL_IMPL_NAMESPACE
static_assert(false, "please specify the implementation namespace (e.g. `vk`)");
#endif

// Denote a function to be defined by HAL implementation.
#define L_IMPL_FN extern
// Denote a structure to be defined by HAL implementation.
#define L_IMPL_STRUCT

namespace liong {

namespace HAL_IMPL_NAMESPACE {

// Initialize the implementation of the HAL.
L_IMPL_FN void initialize();

// Generate Human-readable string to describe the properties and capabilities
// of the device at index `idx`. If there is no device at `idx`, an empty string
// is returned.
L_IMPL_FN std::string desc_dev(uint32_t idx);



struct ContextConfig {
  // Human-readable label of the context.
  std::string label;
  // Index of the device.
  uint32_t dev_idx;
};
L_IMPL_STRUCT struct Context;
L_IMPL_FN Context create_ctxt(const ContextConfig& cfg);
L_IMPL_FN void destroy_ctxt(Context& ctxt);
L_IMPL_FN const ContextConfig& get_ctxt_cfg(const Context& ctxt);



enum MemoryAccess {
  L_MEMORY_ACCESS_NONE = 0b00,
  L_MEMORY_ACCESS_READ_ONLY = 0b01,
  L_MEMORY_ACCESS_WRITE_ONLY = 0b10,
  L_MEMORY_ACCESS_READ_WRITE = 0b11,
};

// Calculate a minimal size of allocation that guarantees that we can sub-
// allocate an address-aligned memory of `size`.
constexpr size_t align_size(size_t size, size_t align) {
  return (size + (align - 1));
}
// Align pointer address to the next aligned address. This funciton assumes that
// `align` is a power-of-2.
constexpr size_t align_addr(size_t size, size_t align) {
  return (size + (align - 1)) & (~(align - 1));
}

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
      throw std::logic_error("not implemented yet");
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
#define L_MAKE_VEC(ncomp, fmt)                                                 \
  ((uint8_t)((ncomp - 1) << 6) | (uint8_t)fmt)
#define L_DEF_FMT(name, ncomp, fmt)                                            \
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



// Describes a buffer.
struct BufferConfig {
  // Human-readable label of the buffer.
  std::string label;
  MemoryAccess host_access;
  MemoryAccess dev_access;
  // Size of buffer allocation, or minimal size of buffer allocation if the
  // buffer has variable size. MUST NOT be zero.
  size_t size;
  // Buffer base address alignment requirement. Zero is treated as one in this
  // field.
  size_t align;
  // If true, the buffer is used as a uniform buffer or constant buffer.
  // Otherwise, the buffer is bound as a storage buffer and can have variable
  // length in compute shaders.
  bool is_const;
};
L_IMPL_STRUCT struct Buffer;
L_IMPL_FN Buffer create_buf(const Context& ctxt, const BufferConfig& buf_cfg);
L_IMPL_FN void destroy_buf(Buffer& buf);
L_IMPL_FN const BufferConfig& get_buf_cfg(const Buffer& buf);

struct BufferView {
  const Buffer* buf; // Lifetime bound.
  size_t offset;
  size_t size;
};

L_IMPL_FN void map_mem(
  const BufferView& dst,
  void*& mapped,
  MemoryAccess map_access
);
L_IMPL_FN void unmap_mem(
  const BufferView& buf,
  void* mapped
);



// Describe a row-major 2D image.
struct ImageConfig {
  // Human-readable label of the image.
  std::string label;
  MemoryAccess host_access;
  MemoryAccess dev_access;
  // Number of rows in the image.
  size_t nrow;
  // Number of columns in the image.
  size_t ncol;
  // Stride between base addresses of rows.
  size_t pitch;
  // Pixel format of the image.
  PixelFormat fmt;
  // If true, the image is bound as a readonly storage image. Otherwise the
  // image is used as a writeonly or read-weite storage image.
  bool is_const;
};
L_IMPL_STRUCT struct Image;
L_IMPL_FN Image create_img(const Context& ctxt, const ImageConfig& img_cfg);
L_IMPL_FN void destroy_img(Image& img);
L_IMPL_FN const ImageConfig& get_img_cfg(const Image& img);

struct ImageView {
  const Image* img; // Lifetime bound.
  uint32_t row_offset;
  uint32_t col_offset;
  uint32_t nrow;
  uint32_t ncol;
};

struct DispatchSize {
  uint32_t x, y, z;
};
enum ResourceType {
  L_RESOURCE_TYPE_BUFFER,
  L_RESOURCE_TYPE_IMAGE,
};
struct ResourceConfig {
  // Type of the resource.
  ResourceType rsc_ty;
  // Whether the resource is a readonly (uniform) resource. A constant resource
  // can have better IO efficiency while must have a fixed size.
  bool is_const;
};
// A device program to be feeded in a `Transaction`.
struct ComputeTaskConfig {
  // Human-readable label of the task.
  std::string label;
  // Name of the entry point. Ignored if the platform does not require to
  // the entry point name.
  std::string entry_name;
  // Code of the task program; will not be copied to the created `Task`.
  // Accepting SPIR-V for Vulkan.
  const void* code;
  // Size of code of the task program in bytes.
  size_t code_size;
  // The resources to be allocated.
  const ResourceConfig* rsc_cfgs;
  // Number of resources to be allocated.
  size_t nrsc_cfg;
  // Local group size; number of threads in a workgroup.
  DispatchSize workgrp_size;
};
L_IMPL_STRUCT struct Task;
L_IMPL_FN Task create_comp_task(
  const Context& ctxt,
  const ComputeTaskConfig& cfg
);
L_IMPL_FN void destroy_task(Task& task);



L_IMPL_STRUCT struct ResourcePool;
L_IMPL_FN ResourcePool create_rsc_pool(
  const Context& ctxt,
  const Task& task
);
L_IMPL_FN void destroy_rsc_pool(ResourcePool& rsc_pool);
L_IMPL_FN void bind_pool_rsc(
  ResourcePool& rsc_pool,
  uint32_t idx,
  const BufferView& buf_view
);
L_IMPL_FN void bind_pool_rsc(
  ResourcePool& rsc_pool,
  uint32_t idx,
  const ImageView& img_view
);



struct Command;
// A wrapping of reusable commands. The resources used in commands of a
// transaction MUST be kept alive during transaction's entire lifetime. A
// transaction MUST NOT inline another transaction in its commands.
L_IMPL_STRUCT struct Transaction;
L_IMPL_FN Transaction create_transact(
  const Context& ctxt,
  const Command* cmds,
  size_t ncmd
);
L_IMPL_FN void destroy_transact(Transaction& transact);



enum CommandType {
  L_COMMAND_TYPE_INLINE_TRANSACTION,
  L_COMMAND_TYPE_COPY_BUFFER_TO_IMAGE,
  L_COMMAND_TYPE_COPY_IMAGE_TO_BUFFER,
  L_COMMAND_TYPE_COPY_BUFFER,
  L_COMMAND_TYPE_COPY_IMAGE,
  L_COMMAND_TYPE_DISPATCH,
};
struct Command {
  CommandType cmd_ty;
  union {
    struct {
      const Transaction* transact;
    } cmd_inline_transact;
    struct {
      const BufferView * src;
      const ImageView* dst;
    } cmd_copy_buf2img;
    struct {
      const ImageView* src;
      const BufferView* dst;
    } cmd_copy_img2buf;
    struct {
      const BufferView* src;
      const BufferView* dst;
    } cmd_copy_buf;
    struct {
      const ImageView* src;
      const ImageView* dst;
    } cmd_copy_img;
    struct {
      const Task* task;
      const ResourcePool* rsc_pool;
      DispatchSize nworkgrp;
    } cmd_dispatch;
  };
};

inline Command cmd_inline_transact(const Transaction& transact) {
  Command cmd;
  cmd.cmd_ty = L_COMMAND_TYPE_INLINE_TRANSACTION;
  cmd.cmd_inline_transact.transact = &transact;
  return cmd;
}

// Copy data from a buffer to an image.
inline Command cmd_copy_buf2img(const BufferView& src, const ImageView& dst) {
  Command cmd;
  cmd.cmd_ty = L_COMMAND_TYPE_COPY_BUFFER_TO_IMAGE;
  cmd.cmd_copy_buf2img.src = &src;
  cmd.cmd_copy_buf2img.dst = &dst;
  return cmd;
}
// Copy data from an image to a buffer.
inline Command cmd_copy_img2buf(const ImageView& src, const BufferView& dst) {
  Command cmd;
  cmd.cmd_ty = L_COMMAND_TYPE_COPY_IMAGE_TO_BUFFER;
  cmd.cmd_copy_img2buf.src = &src;
  cmd.cmd_copy_img2buf.dst = &dst;
  return cmd;
}
// Copy data from a buffer to another buffer.
inline Command cmd_copy_buf(const BufferView& src, const BufferView& dst) {
  Command cmd;
  cmd.cmd_ty = L_COMMAND_TYPE_COPY_BUFFER;
  cmd.cmd_copy_buf.src = &src;
  cmd.cmd_copy_buf.dst = &dst;
  return cmd;
}
// Copy data from an image to another image.
inline Command cmd_copy_img(const ImageView& src, const ImageView& dst) {
  Command cmd;
  cmd.cmd_ty = L_COMMAND_TYPE_COPY_IMAGE;
  cmd.cmd_copy_img.src = &src;
  cmd.cmd_copy_img.dst = &dst;
  return cmd;
}

// Dispatch a task to the transaction.
inline Command cmd_dispatch(
  const Task& task,
  const ResourcePool& rsc_pool,
  DispatchSize nworkgrp) {
  Command cmd;
  cmd.cmd_ty = L_COMMAND_TYPE_DISPATCH;
  cmd.cmd_dispatch.task = &task;
  cmd.cmd_dispatch.rsc_pool = &rsc_pool;
  cmd.cmd_dispatch.nworkgrp = nworkgrp;
  return cmd;
}



L_IMPL_STRUCT struct CommandDrain;
L_IMPL_FN CommandDrain create_cmd_drain(const Context& ctxt);
L_IMPL_FN void destroy_cmd_drain(CommandDrain& cmd_drain);
// Submit commands and inlined transactions to the device for execution.
L_IMPL_FN void submit_cmds(
  CommandDrain& cmd_drain,
  const Command* cmds,
  size_t ncmd
);
// Wait until the command drain consumed all the commands and finished
// execution.
L_IMPL_FN void wait_cmd_drain(CommandDrain& cmd_drain);

namespace ext {

std::vector<uint8_t> load_code(const std::string& prefix);

} // namespace ext

} // namespace HAL_IMPL_NAMESPACE

} // namespace liong

#undef L_IMPL_FN
#undef L_IMPL_STRUCT
