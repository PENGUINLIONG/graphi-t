﻿// # Hardware abstraction layer
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
#include "px-fmt.hpp"

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



enum MemoryAccessBits {
  L_MEMORY_ACCESS_READ_BIT = 0b01,
  L_MEMORY_ACCESS_WRITE_BIT = 0b10,
};
typedef uint32_t MemoryAccess;

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

enum BufferUsageBits {
  L_BUFFER_USAGE_STAGING_BIT = (1 << 0),
  L_BUFFER_USAGE_UNIFORM_BIT = (1 << 1),
  L_BUFFER_USAGE_STORAGE_BIT = (1 << 2),
  L_BUFFER_USAGE_VERTEX_BIT = (1 << 3),
  L_BUFFER_USAGE_INDEX_BIT = (1 << 4),
};
typedef uint32_t BufferUsage;
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
  // Usage of the buffer.
  BufferUsage usage;
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
inline BufferView make_buf_view(const Buffer& buf, size_t offset, size_t size) {
  BufferView out {};
  out.buf = &buf;
  out.offset = offset;
  out.size = size;
  return out;
}
inline BufferView make_buf_view(const Buffer& buf) {
  const BufferConfig& buf_cfg = get_buf_cfg(buf);
  return make_buf_view(buf, 0, buf_cfg.size);
}

L_IMPL_FN void map_buf_mem(
  const BufferView& dst,
  MemoryAccess map_access,
  void*& mapped
);
L_IMPL_FN void unmap_buf_mem(
  const BufferView& buf,
  void* mapped
);



enum ImageUsageBits {
  L_IMAGE_USAGE_NONE = 0,
  L_IMAGE_USAGE_STAGING_BIT = (1 << 0),
  L_IMAGE_USAGE_SAMPLED_BIT = (1 << 1),
  L_IMAGE_USAGE_STORAGE_BIT = (1 << 2),
  L_IMAGE_USAGE_ATTACHMENT_BIT = (1 << 3),
  L_IMAGE_USAGE_PRESENT_BIT = (1 << 4),
};
typedef uint32_t ImageUsage;
// Describe a row-major 2D image.
struct ImageConfig {
  // Human-readable label of the image.
  std::string label;
  MemoryAccess host_access;
  MemoryAccess dev_access;
  // Number of rows, or height of the image.
  uint32_t height;
  // Number of columns, or width of the image.
  uint32_t width;
  // Pixel format of the image.
  PixelFormat fmt;
  // Usage of the image.
  ImageUsage usage;
};
L_IMPL_STRUCT struct Image;
L_IMPL_FN Image create_img(const Context& ctxt, const ImageConfig& img_cfg);
L_IMPL_FN void destroy_img(Image& img);
L_IMPL_FN const ImageConfig& get_img_cfg(const Image& img);

struct ImageView {
  const Image* img; // Lifetime bound.
  uint32_t x_offset;
  uint32_t y_offset;
  uint32_t width;
  uint32_t height;
};
inline ImageView make_img_view(
  const Image& img,
  uint32_t x_offset,
  uint32_t y_offset,
  uint32_t width,
  uint32_t height
) {
  ImageView out {};
  out.img = &img;
  out.x_offset = x_offset;
  out.y_offset = y_offset;
  out.width = width;
  out.height = height;
  return out;
}
inline ImageView make_img_view(const Image& img) {
  const ImageConfig& img_cfg = get_img_cfg(img);
  return make_img_view(img, 0, 0, img_cfg.width, img_cfg.height);
}

L_IMPL_FN void map_img_mem(
  const ImageView& img,
  MemoryAccess map_access,
  void*& mapped,
  size_t& row_pitch
);
L_IMPL_FN void unmap_img_mem(
  const ImageView& img,
  void* mapped
);



L_IMPL_STRUCT struct RenderPass;
L_IMPL_FN RenderPass create_pass(
  const Context& ctxt,
  const Image& img
);
L_IMPL_FN void destroy_pass(RenderPass& pass);

enum DepthImageUsageBits {
  L_DEPTH_IMAGE_USAGE_NONE = 0,
  L_DEPTH_IMAGE_USAGE_SAMPLED_BIT = (1 << 0),
  L_DEPTH_IMAGE_USAGE_ATTACHMENT_BIT = (1 << 1),
};
typedef uint32_t DepthImageUsage;
struct DepthImageConfig {
  std::string label;
  // Width of the depth image. When used, the image size should match color
  // attachment size.
  uint32_t width;
  // Height of the depth image. When used, the image size should match color
  // attachment size.
  uint32_t height;
  // Pixel format of depth image.
  DepthFormat fmt;
  // Usage of the depth image.
  DepthImageUsage usage;
};
L_IMPL_STRUCT struct DepthImage;
L_IMPL_FN DepthImage create_depth_img(
  const Context& ctxt,
  const DepthImageConfig& depth_img_cfg
);
L_IMPL_FN void destroy_depth_img(DepthImage& depth_img);
L_IMPL_FN const DepthImageConfig& get_depth_img_cfg(const DepthImage& depth_img);



struct DispatchSize {
  uint32_t x, y, z;
};
enum ResourceType {
  L_RESOURCE_TYPE_UNIFORM_BUFFER,
  L_RESOURCE_TYPE_STORAGE_BUFFER,
  L_RESOURCE_TYPE_SAMPLED_IMAGE,
  L_RESOURCE_TYPE_STORAGE_IMAGE,
};
// A device program to be feeded in a `Transaction`.
struct ComputeTaskConfig {
  // Human-readable label of the task.
  std::string label;
  // Name of the entry point. Ignored if the platform does not require an entry
  // point name.
  std::string entry_name;
  // Code of the task program; will not be copied to the created `Task`.
  // Accepting SPIR-V for Vulkan.
  const void* code;
  // Size of code of the task program in bytes.
  size_t code_size;
  // The resources to be allocated.
  std::vector<ResourceType> rsc_tys;
  // Local group size; number of threads in a workgroup.
  DispatchSize workgrp_size;
};

enum AttachmentType {
  L_ATTACHMENT_TYPE_COLOR,
  L_ATTACHMENT_TYPE_DEPTH,
};
enum AttachmentAccess {
  // Don't care about the access pattern
  L_ATTACHMENT_ACCESS_DONT_CARE = 0b0000,
  // When the attachment is read-accessed, the previous value of the pixel is
  // ignored and is overwritten by a specified value.
  L_ATTACHMENT_ACCESS_CLEAR = 0b0001,
  // When the attachment is read-accessed, the previous value of the pixel is
  // loaded from memory.
  L_ATTACHMENT_ACCESS_LOAD = 0b0010,
  // When the attachment is write-accessed, the shader output is written to
  // memory.
  L_ATTACHMENT_ACCESS_STORE = 0b0100,
  // When the attachment is read-accessed, the previous value of the pixel is
  // loaded as subpass data.
  // TOOD: (penguinliong) Implement framebuffer fetch.
  L_ATTACHMENT_ACCESS_FETCH = 0b1000,
};
struct AttachmentConfig {
  // Attachment access pattern.
  AttachmentAccess attm_access;
  // Attachment type.
  AttachmentType attm_ty;
  // The image to be used as an attachment.
  union {
    const Image* color_img;
    const DepthImage* depth_img;
  };
};
struct RenderPassConfig {
  std::string label;
  // Width of attachments.
  uint32_t width;
  // Height of attachments.
  uint32_t height;
  // Configurations of attachments that will be used in the render pass.
  std::vector<AttachmentConfig> attm_cfgs;
};
L_IMPL_STRUCT struct RenderPass;
RenderPass create_pass(const Context& ctxt, const RenderPassConfig& cfg);
void destroy_pass(RenderPass& pass);

enum Topology {
  L_TOPOLOGY_POINT = 1,
  L_TOPOLOGY_LINE = 2,
  L_TOPOLOGY_TRIANGLE = 3,
};
enum VertexInputRate {
  L_VERTEX_INPUT_RATE_VERTEX,
  L_VERTEX_INPUT_RATE_INSTANCE,
};
struct VertexInput {
  PixelFormat fmt;
  VertexInputRate rate;
};
struct GraphicsTaskConfig {
  // Human-readable label of the task.
  std::string label;
  // Name of the vertex stage entry point. Ignored if the platform does not
  // require an entry point name.
  std::string vert_entry_name;
  // Code of the vertex stage of the task program; will not be copied to the
  // created `Task`. Accepting SPIR-V for Vulkan.
  const void* vert_code;
  // Size of code of the vertex stage of the task program in bytes.
  size_t vert_code_size;
  // Name of the fragment stage entry point. Ignored if the platform does not
  // require an entry point name.
  std::string frag_entry_name;
  // Code of the fragment stage of the task program; will not be copied to the
  // created `Task`. Accepting SPIR-V for Vulkan.
  const void* frag_code;
  // Size of code of the fragment stage of the task program in bytes.
  size_t frag_code_size;
  // Topology of vertex inputs to be assembled.
  Topology topo;
  // Vertex vertex input specifications.
  std::vector<VertexInput> vert_inputs;
  // Resources to be allocated.
  std::vector<ResourceType> rsc_tys;
};

L_IMPL_STRUCT struct Task;
L_IMPL_FN Task create_comp_task(
  const Context& ctxt,
  const ComputeTaskConfig& cfg
);
L_IMPL_FN Task create_graph_task(
  const RenderPass& pass,
  const GraphicsTaskConfig& cfg
);
L_IMPL_FN void destroy_task(Task& task);



struct ResourceView {
  BufferView buf_view;
  ImageView img_view;

  inline ResourceView(BufferView buf_view) : buf_view(buf_view), img_view() {}
  inline ResourceView(ImageView img_view) : buf_view(), img_view(img_view) {}
};
// Instanced invocation of a compute task, a.k.a. a dispatch.
struct ComputeInvocationConfig {
  std::string label;
  // Resources bound to this invocation.
  std::vector<ResourceView> rsc_views;
  // Number of workgroups dispatched in this invocation.
  DispatchSize workgrp_count;
};
// Instanced invocation of a graphics task, a.k.a. a draw call.
struct GraphicsInvocationConfig {
  std::string label;
  // Resources bound to this invocation.
  std::vector<ResourceView> rsc_views;
  // Number of instances to be drawn.
  uint32_t ninst;
  // Vertex buffer for drawing.
  std::vector<BufferView> vert_bufs;
  // Number of vertices to be drawn in this draw. If `nidx` is non-zero, `nvert`
  // MUST be zero.
  uint32_t nvert;
  // Index buffer for vertex indexing.
  BufferView idx_buf;
  // Number of indices to be drawn in this draw. If `nvert` is non-zero, `nidx`
  // MUST be zero.
  uint32_t nidx;
};
L_IMPL_STRUCT struct Invocation;
L_IMPL_FN Invocation create_comp_invoke(
  const Task& task,
  const ComputeInvocationConfig& cfg
);
L_IMPL_FN Invocation create_graph_invoke(
  const Task& task,
  const GraphicsInvocationConfig& cfg
);
L_IMPL_FN void destroy_invoke(Invocation& invoke);



struct Command;
// A wrapping of reusable commands. The resources used in commands of a
// transaction MUST be kept alive during transaction's entire lifetime. A
// transaction MUST NOT inline another transaction in its commands.
L_IMPL_STRUCT struct Transaction;
L_IMPL_FN Transaction create_transact(
  const std::string& label,
  const Context& ctxt,
  const Command* cmds,
  size_t ncmd
);
L_IMPL_FN void destroy_transact(Transaction& transact);



// A timestamp object written by the GPU, usually for precise timing.
L_IMPL_STRUCT struct Timestamp;
L_IMPL_FN Timestamp create_timestamp(const Context& ctxt);
L_IMPL_FN void destroy_timestamp(Timestamp& timestamp);
L_IMPL_FN double get_timestamp_result_us(const Timestamp& timestamp);

/*

L_IMPL_STRUCT struct Invocation;
L_IMPL_FN Invocation create_invocation(const Context& ctxt);
L_IMPL_FN void destroy_invocation(const Context& ctxt);



struct GraphicsCommandBuilder {
  Command build();
};

enum CommandParameterType {
  L_COMMAND_PARAMETER_TYPE_I32,
  L_COMMAND_PARAMETER_TYPE_U32,
  L_COMMAND_PARAMETER_TYPE_F32,
  L_COMMAND_PARAMETER_TYPE_BUFFER,
  L_COMMAND_PARAMETER_TYPE_IMAGE,
  L_COMMAND_PARAMETER_TYPE_GRAPHICS_TASK,
  L_COMMAND_PARAMETER_TYPE_COMPUTE_TASK,
};

struct CommandParameter {
  CommandParameterType param_ty;
  union {
    struct {
      BufferUsage buf_usage;
      MemoryAccess buf_access;
    } buf;
    struct {
      ImageUsage img_usage;
      MemoryAccess img_access;
    } img;
  } contract;
};
struct CompositeCommand {
  std::string label;
  std::vector<Command> cmds;
  std::vector<CommandParameter> params;
};



enum ComputeCommandParameterClass {
  L_COMPUTE_COMMAND_PARAMETER_CLASS_WORKGROUP_COUNT, // u32
  L_COMPUTE_COMMAND_PARAMETER_CLASS_RESOURCE, // Buffer | Image
};
struct ComputeCommand {
  const Task* task;
  uint32_t nworkgrp[3];
  std::vector<Resource>;
};



enum GraphicsCommandParameterType {
  L_GRAPHICS_COMMAND_PARAMETER_TYPE_VERTEX_COUNT,
  L_GRAPHICS_COMMAND_PARAMETER_TYPE_VERTEX_BUFFER,
  L_GRAPHICS_COMMAND_PARAMETER_TYPE_INDEX_COUNT,
  L_GRAPHICS_COMMAND_PARAMETER_TYPE_INDEX_BUFFER,
  L_GRAPHICS_COMMAND_PARAMETER_TYPE_COLOR_ATTACHMENT,
  L_GRAPHICS_COMMAND_PARAMETER_TYPE_DEPTH_STENCIL_ATTACHMENT,

};




*/

enum CommandType {
  L_COMMAND_TYPE_SET_SUBMIT_TYPE,
  L_COMMAND_TYPE_INLINE_TRANSACTION,
  L_COMMAND_TYPE_COPY_BUFFER_TO_IMAGE,
  L_COMMAND_TYPE_COPY_IMAGE_TO_BUFFER,
  L_COMMAND_TYPE_COPY_BUFFER,
  L_COMMAND_TYPE_COPY_IMAGE,
  L_COMMAND_TYPE_INVOKE,
  L_COMMAND_TYPE_WRITE_TIMESTAMP,
  L_COMMAND_TYPE_BUFFER_BARRIER,
  L_COMMAND_TYPE_IMAGE_BARRIER,
  L_COMMAND_TYPE_BEGIN_RENDER_PASS,
  L_COMMAND_TYPE_END_RENDER_PASS,
  L_COMMAND_TYPE_DEPTH_IMAGE_BARRIER,
};
enum SubmitType {
  L_SUBMIT_TYPE_COMPUTE,
  L_SUBMIT_TYPE_GRAPHICS,
  L_SUBMIT_TYPE_ANY = ~((uint32_t)0),
};
struct Command {
  CommandType cmd_ty;
  union {
    struct {
      SubmitType submit_ty;
    } cmd_set_submit_ty;
    struct {
      const Transaction* transact;
    } cmd_inline_transact;
    struct {
      BufferView src;
      ImageView dst;
    } cmd_copy_buf2img;
    struct {
      ImageView src;
      BufferView dst;
    } cmd_copy_img2buf;
    struct {
      BufferView src;
      BufferView dst;
    } cmd_copy_buf;
    struct {
      ImageView src;
      ImageView dst;
    } cmd_copy_img;
    struct {
      const Invocation* invoke;
    } cmd_invoke;
    struct {
      const Timestamp* timestamp;
    } cmd_write_timestamp;
    struct {
      const Buffer* buf;
      MemoryAccess src_dev_access;
      MemoryAccess dst_dev_access;
      ImageUsage src_usage;
      ImageUsage dst_usage;
    } cmd_buf_barrier;
    struct {
      const Image* img;
      MemoryAccess src_dev_access;
      MemoryAccess dst_dev_access;
      ImageUsage src_usage;
      ImageUsage dst_usage;
    } cmd_img_barrier;
    struct {
      const RenderPass* pass;
      bool draw_inline;
    } cmd_begin_pass;
    struct {
      const RenderPass* pass;
    } cmd_end_pass;
    struct {
      const DepthImage* depth_img;
      MemoryAccess src_dev_access;
      MemoryAccess dst_dev_access;
      DepthImageUsage src_usage;
      DepthImageUsage dst_usage;
    } cmd_depth_img_barrier;
  };
};

inline Command cmd_inline_transact(const Transaction& transact) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_INLINE_TRANSACTION;
  cmd.cmd_inline_transact.transact = &transact;
  return cmd;
}

// Copy data from a buffer to an image.
inline Command cmd_copy_buf2img(const BufferView& src, const ImageView& dst) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_COPY_BUFFER_TO_IMAGE;
  cmd.cmd_copy_buf2img.src = src;
  cmd.cmd_copy_buf2img.dst = dst;
  return cmd;
}
// Copy data from an image to a buffer.
inline Command cmd_copy_img2buf(const ImageView& src, const BufferView& dst) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_COPY_IMAGE_TO_BUFFER;
  cmd.cmd_copy_img2buf.src = src;
  cmd.cmd_copy_img2buf.dst = dst;
  return cmd;
}
// Copy data from a buffer to another buffer.
inline Command cmd_copy_buf(const BufferView& src, const BufferView& dst) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_COPY_BUFFER;
  cmd.cmd_copy_buf.src = src;
  cmd.cmd_copy_buf.dst = dst;
  return cmd;
}
// Copy data from an image to another image.
inline Command cmd_copy_img(const ImageView& src, const ImageView& dst) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_COPY_IMAGE;
  cmd.cmd_copy_img.src = src;
  cmd.cmd_copy_img.dst = dst;
  return cmd;
}

// Realize an invocation.
inline Command cmd_invoke(const Invocation& invoke) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_INVOKE;
  cmd.cmd_invoke.invoke = &invoke;
  return cmd;
}

inline Command cmd_write_timestamp(const Timestamp& timestamp) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_WRITE_TIMESTAMP;
  cmd.cmd_write_timestamp.timestamp = &timestamp;
  return cmd;
}

inline Command cmd_set_submit_ty(SubmitType submit_ty) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_SET_SUBMIT_TYPE;
  cmd.cmd_set_submit_ty.submit_ty = submit_ty;
  return cmd;
}

inline Command cmd_buf_barrier(
  const Buffer& buf,
  BufferUsage src_usage,
  BufferUsage dst_usage,
  MemoryAccess src_dev_access,
  MemoryAccess dst_dev_access
) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_BUFFER_BARRIER;
  cmd.cmd_buf_barrier.buf = &buf;
  cmd.cmd_buf_barrier.src_dev_access = src_dev_access;
  cmd.cmd_buf_barrier.dst_dev_access = dst_dev_access;
  cmd.cmd_buf_barrier.src_usage = src_usage;
  cmd.cmd_buf_barrier.dst_usage = dst_usage;
  return cmd;
}
inline Command cmd_img_barrier(
  const Image& img,
  ImageUsage src_usage,
  ImageUsage dst_usage,
  MemoryAccess src_dev_access,
  MemoryAccess dst_dev_access
) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_IMAGE_BARRIER;
  cmd.cmd_img_barrier.img = &img;
  cmd.cmd_img_barrier.src_dev_access = src_dev_access;
  cmd.cmd_img_barrier.dst_dev_access = dst_dev_access;
  cmd.cmd_img_barrier.src_usage = src_usage;
  cmd.cmd_img_barrier.dst_usage = dst_usage;
  return cmd;
}
inline Command cmd_begin_pass(
  const RenderPass& pass,
  bool draw_inline
) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_BEGIN_RENDER_PASS;
  cmd.cmd_begin_pass.pass = &pass;
  cmd.cmd_begin_pass.draw_inline = draw_inline;
  return cmd;
}
inline Command cmd_end_pass(
  const RenderPass& pass
) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_END_RENDER_PASS;
  cmd.cmd_end_pass.pass = &pass;
  return cmd;
}
inline Command cmd_depth_img_barrier(
  const DepthImage& depth_img,
  DepthImageUsage src_usage,
  DepthImageUsage dst_usage,
  MemoryAccess src_dev_access,
  MemoryAccess dst_dev_access
) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_DEPTH_IMAGE_BARRIER;
  cmd.cmd_depth_img_barrier.depth_img = &depth_img;
  cmd.cmd_depth_img_barrier.src_dev_access = src_dev_access;
  cmd.cmd_depth_img_barrier.dst_dev_access = dst_dev_access;
  cmd.cmd_depth_img_barrier.src_usage = src_usage;
  cmd.cmd_depth_img_barrier.dst_usage = dst_usage;
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
