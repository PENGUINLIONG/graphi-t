// # Hardware abstraction layer
// @PENGUINLIONG
//
// This file defines all APIs to be implemented by each platform and provides
// interfacing structures to the most common extent.
//
// Before implementing a HAL, please include `common.hpp` at the beginning of
// your header.
#pragma once
#include <array>
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include "gft/assert.hpp"
#include "gft/fmt.hpp"

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
// Finalize the implementation.
L_IMPL_FN void finalize();

// Generate Human-readable string to describe the properties and capabilities
// of the device at index `idx`. If there is no device at `idx`, an empty string
// is returned.
L_IMPL_FN std::string desc_dev(uint32_t idx);



// A batch of works dispatched to the device.
L_IMPL_STRUCT struct Transaction;
// Check whether the transaction is finished. `true` is returned if so.
L_IMPL_FN bool is_transact_done(const Transaction& transact);
// Wait the invocation submitted to device for execution. Returns immediately if
// the invocation has already been waited.
L_IMPL_FN void wait_transact(const Transaction& transact);



struct ContextConfig {
  // Human-readable label of the context.
  std::string label;
  // Index of the device.
  uint32_t dev_idx;
};
struct ContextWindowsConfig {
  std::string label;
  // Index of the device.
  uint32_t dev_idx;
  // Instance handle, aka `HINSTANCE`.
  const void* hinst;
  // Window handle, aka `HWND`.
  const void* hwnd;
};
struct ContextAndroidConfig {
  std::string label;
  // Index of the device.
  uint32_t dev_idx;
  // Android native window, `ANativeWindow`.
  const void* native_wnd;
};
struct ContextMetalConfig {
  std::string label;
  uint32_t dev_idx;
  const void* metal_layer;
};

L_IMPL_STRUCT struct Context;
L_IMPL_FN Context create_ctxt(const ContextConfig& cfg);
L_IMPL_FN Context create_ctxt_windows(const ContextWindowsConfig& cfg);
L_IMPL_FN Context create_ctxt_android(const ContextAndroidConfig& cfg);
L_IMPL_FN Context create_ctxt_metal(const ContextMetalConfig& cfg);



enum MemoryAccessBits {
  L_MEMORY_ACCESS_READ_BIT = 0b01,
  L_MEMORY_ACCESS_WRITE_BIT = 0b10,
};
typedef uint32_t MemoryAccess;

enum BufferUsageBits {
  L_BUFFER_USAGE_NONE = 0,
  L_BUFFER_USAGE_TRANSFER_SRC_BIT = (1 << 0),
  L_BUFFER_USAGE_TRANSFER_DST_BIT = (1 << 1),
  L_BUFFER_USAGE_UNIFORM_BIT = (1 << 2),
  L_BUFFER_USAGE_STORAGE_BIT = (1 << 3),
  L_BUFFER_USAGE_VERTEX_BIT = (1 << 4),
  L_BUFFER_USAGE_INDEX_BIT = (1 << 5),
};
typedef uint32_t BufferUsage;
// Describes a buffer.
struct BufferConfig {
  // Human-readable label of the buffer.
  std::string label;
  MemoryAccess host_access;
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
L_IMPL_FN void read_buf_mem(
  const BufferView& buf,
  void* data,
  size_t size
);
L_IMPL_FN void write_buf_mem(
  const BufferView& buf,
  const void* data,
  size_t size
);



enum ImageUsageBits {
  L_IMAGE_USAGE_NONE = 0,
  L_IMAGE_USAGE_TRANSFER_SRC_BIT = (1 << 0),
  L_IMAGE_USAGE_TRANSFER_DST_BIT = (1 << 1),
  L_IMAGE_USAGE_SAMPLED_BIT = (1 << 2),
  L_IMAGE_USAGE_STORAGE_BIT = (1 << 3),
  L_IMAGE_USAGE_ATTACHMENT_BIT = (1 << 4),
  L_IMAGE_USAGE_SUBPASS_DATA_BIT = (1 << 5),
  L_IMAGE_USAGE_TILE_MEMORY_BIT = (1 << 6),
  L_IMAGE_USAGE_PRESENT_BIT = (1 << 7),
};
typedef uint32_t ImageUsage;
// Describe a row-major 2D image.
struct ImageConfig {
  // Human-readable label of the image.
  std::string label;
  // Width of the image.
  uint32_t width;
  // Height of the image, or zero if not 2D or 3D texture.
  uint32_t height;
  // Depth of the image, or zero if not 3D texture.
  uint32_t depth;
  // Pixel format of the image.
  fmt::Format fmt;
  // Color space of the image. Only linear and srgb are valid and it only
  // affects how the image data is interpreted on reads.
  fmt::ColorSpace cspace;
  // Usage of the image.
  ImageUsage usage;
};
L_IMPL_STRUCT struct Image;
L_IMPL_FN Image create_img(const Context& ctxt, const ImageConfig& img_cfg);
L_IMPL_FN const ImageConfig& get_img_cfg(const Image& img);

enum ImageSampler {
  L_IMAGE_SAMPLER_LINEAR,
  L_IMAGE_SAMPLER_NEAREST,
  L_IMAGE_SAMPLER_ANISOTROPY_4,
};
struct ImageView {
  const Image* img; // Lifetime bound.
  uint32_t x_offset;
  uint32_t y_offset;
  uint32_t width;
  uint32_t height;
  uint32_t depth;
  ImageSampler sampler;
};
inline ImageView make_img_view(
  const Image& img,
  uint32_t x_offset,
  uint32_t y_offset,
  uint32_t width,
  uint32_t height,
  uint32_t depth,
  ImageSampler sampler
) {
  ImageView out {};
  out.img = &img;
  out.x_offset = x_offset;
  out.y_offset = y_offset;
  out.width = width;
  out.height = height;
  out.depth = depth;
  out.sampler = sampler;
  return out;
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



enum DepthImageUsageBits {
  L_DEPTH_IMAGE_USAGE_NONE = 0,
  L_DEPTH_IMAGE_USAGE_SAMPLED_BIT = (1 << 0),
  L_DEPTH_IMAGE_USAGE_ATTACHMENT_BIT = (1 << 1),
  L_DEPTH_IMAGE_USAGE_SUBPASS_DATA_BIT = (1 << 2),
  L_DEPTH_IMAGE_USAGE_TILE_MEMORY_BIT = (1 << 3),
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
  fmt::DepthFormat fmt;
  // Usage of the depth image.
  DepthImageUsage usage;
};
L_IMPL_STRUCT struct DepthImage;
L_IMPL_FN DepthImage create_depth_img(
  const Context& ctxt,
  const DepthImageConfig& depth_img_cfg
);
L_IMPL_FN const DepthImageConfig& get_depth_img_cfg(const DepthImage& depth_img);

enum DepthImageSampler {
  L_DEPTH_IMAGE_SAMPLER_LINEAR,
  L_DEPTH_IMAGE_SAMPLER_NEAREST,
  L_DEPTH_IMAGE_SAMPLER_ANISOTROPY_4,
};
struct DepthImageView {
  const DepthImage* depth_img; // Lifetime bound.
  uint32_t x_offset;
  uint32_t y_offset;
  uint32_t width;
  uint32_t height;
  DepthImageSampler sampler;
};
inline DepthImageView make_depth_img_view(
  const DepthImage& depth_img,
  uint32_t x_offset,
  uint32_t y_offset,
  uint32_t width,
  uint32_t height,
  DepthImageSampler sampler
) {
  DepthImageView out {};
  out.depth_img = &depth_img;
  out.x_offset = x_offset;
  out.y_offset = y_offset;
  out.width = width;
  out.height = height;
  out.sampler = sampler;
  return out;
}



struct SwapchainConfig {
  std::string label;
  // Number of image for multibuffering, can be 1, 2 or 3.
  uint32_t nimg;
  // Candidate image color formats. The format is selected based on platform
  // availability.
  std::vector<fmt::Format> fmts;
  // Render output color space. Note that the color space is specified for
  // presentation. The rendering output should always be linear colors.
  fmt::ColorSpace cspace;
};
L_IMPL_STRUCT struct Swapchain;
L_IMPL_FN Swapchain create_swapchain(
  const Context& ctxt,
  const SwapchainConfig& cfg
);
L_IMPL_FN const SwapchainConfig& get_swapchain_cfg(const Swapchain& swapchain);
// Get the surface image for the current frame. Surface image is alive after the
// `acquire_surf_img` transition finishes and before the next presentation
// invocation.
L_IMPL_FN const Image& get_swapchain_img(const Swapchain& swapchain);
L_IMPL_FN uint32_t get_swapchain_img_width(const Swapchain& swapchain);
L_IMPL_FN uint32_t get_swapchain_img_height(const Swapchain& swapchain);



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
  union {
    struct {
      // Color attachment format.
      fmt::Format color_fmt;
      // Color attachment color space.
      fmt::ColorSpace cspace;
    };
    // Depth attachment format.
    fmt::DepthFormat depth_fmt;
  };
};
// TODO: (penguinliong) Multi-subpass rendering.
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

enum Topology {
  L_TOPOLOGY_POINT = 1,
  L_TOPOLOGY_LINE = 2,
  L_TOPOLOGY_TRIANGLE = 3,
  L_TOPOLOGY_TRIANGLE_WIREFRAME = 4,
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



enum ResourceViewType {
  L_RESOURCE_VIEW_TYPE_BUFFER,
  L_RESOURCE_VIEW_TYPE_IMAGE,
  L_RESOURCE_VIEW_TYPE_DEPTH_IMAGE,
};
struct ResourceView {
  ResourceViewType rsc_view_ty;
  BufferView buf_view;
  ImageView img_view;
  DepthImageView depth_img_view;
};
inline ResourceView make_rsc_view(const BufferView& buf_view) {
  ResourceView rsc_view {};
  rsc_view.rsc_view_ty = L_RESOURCE_VIEW_TYPE_BUFFER;
  rsc_view.buf_view = buf_view;
  return rsc_view;
}
inline ResourceView make_rsc_view(const ImageView& img_view) {
  ResourceView rsc_view {};
  rsc_view.rsc_view_ty = L_RESOURCE_VIEW_TYPE_IMAGE;
  rsc_view.img_view = img_view;
  return rsc_view;
}
inline ResourceView make_rsc_view(const DepthImageView& depth_img_view) {
  ResourceView rsc_view {};
  rsc_view.rsc_view_ty = L_RESOURCE_VIEW_TYPE_DEPTH_IMAGE;
  rsc_view.depth_img_view = depth_img_view;
  return rsc_view;
}
struct TransferInvocationConfig {
  std::string label;
  // Data transfer source.
  ResourceView src_rsc_view;
  // Data transfer destination.
  ResourceView dst_rsc_view;
  // Set `true` if the device-side execution time is wanted.
  bool is_timed;
};
// Instanced invocation of a compute task, a.k.a. a dispatch.
struct ComputeInvocationConfig {
  std::string label;
  // Resources bound to this invocation.
  std::vector<ResourceView> rsc_views;
  // Number of workgroups dispatched in this invocation.
  DispatchSize workgrp_count;
  // Set `true` if the device-side execution time is wanted.
  bool is_timed;
};
enum IndexType {
  L_INDEX_TYPE_UINT16,
  L_INDEX_TYPE_UINT32,
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
  // Type of index buffer elements.
  IndexType idx_ty;
  // Number of indices to be drawn in this draw. If `nvert` is non-zero, `nidx`
  // MUST be zero.
  uint32_t nidx;
  // Set `true` if the device-side execution time is wanted.
  bool is_timed;
};
struct RenderPassInvocationConfig {
  std::string label;
  // Attachment feed in order, can be `Image` or `DepthImage` only.
  std::vector<ResourceView> attms;
  // Graphics invocations applied within this render pass.
  std::vector<const struct Invocation*> invokes; // Lifetime bound.
  // Set `true` if the device-side execution time is wanted.
  bool is_timed;
};
struct CompositeInvocationConfig {
  std::string label;
  // Compute or render pass invocations within this composite invocation. Note
  // that graphics invocations cannot be called outside of render passes.
  std::vector<const struct Invocation*> invokes; // Lifetime bound.
  // Set `true` if the device-side execution time is wanted.
  bool is_timed;
};
L_IMPL_STRUCT struct Invocation;
L_IMPL_FN Invocation create_trans_invoke(
  const Context& ctxt,
  const TransferInvocationConfig& cfg
);
L_IMPL_FN Invocation create_comp_invoke(
  const Task& task,
  const ComputeInvocationConfig& cfg
);
L_IMPL_FN Invocation create_graph_invoke(
  const Task& task,
  const GraphicsInvocationConfig& cfg
);
L_IMPL_FN Invocation create_pass_invoke(
  const RenderPass& pass,
  const RenderPassInvocationConfig& cfg
);
// Present the content written to the current surface image.
L_IMPL_FN Invocation create_present_invoke(
  const Swapchain& swapchain
);
L_IMPL_FN Invocation create_composite_invoke(
  const Context& ctxt,
  const CompositeInvocationConfig& cfg
);
// Get the execution time of the last WAITED invocation.
L_IMPL_FN double get_invoke_time_us(const Invocation& invoke);
// Pre-encode the invocation commands to reduce host-side overhead on constant
// device-side procedures.
L_IMPL_FN void bake_invoke(Invocation& invoke);
// Create a device transactiona and submit `invoke` to device for execution.
L_IMPL_FN Transaction submit_invoke(const Invocation& invoke);

} // namespace HAL_IMPL_NAMESPACE

} // namespace liong

#undef L_IMPL_FN
#undef L_IMPL_STRUCT
