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
#include <chrono>
#include <cstdint>
#include <vector>
#include "gft/fmt.hpp"

namespace liong {
namespace hal {

// - [Interfaces] --------------------------------------------------------------

#define L_DEF_INTERFACE_TYPE(name)                                             \
  struct name;                                                                 \
  typedef std::shared_ptr<name> name##Ref;

L_DEF_INTERFACE_TYPE(Instance);
L_DEF_INTERFACE_TYPE(Context);
L_DEF_INTERFACE_TYPE(Buffer);
L_DEF_INTERFACE_TYPE(Image);
L_DEF_INTERFACE_TYPE(DepthImage);
L_DEF_INTERFACE_TYPE(Swapchain);
L_DEF_INTERFACE_TYPE(Task);
L_DEF_INTERFACE_TYPE(RenderPass);
L_DEF_INTERFACE_TYPE(Invocation);

#undef L_DEF_INTERFACE_TYPE

// - [Constants] ---------------------------------------------------------------

const uint32_t SPIN_INTERVAL = 30'000;

// - [Definitions] -------------------------------------------------------------

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

struct BufferView {
  BufferRef buf;
  size_t offset;
  size_t size;
};

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

enum ImageSampler {
  L_IMAGE_SAMPLER_LINEAR,
  L_IMAGE_SAMPLER_NEAREST,
  L_IMAGE_SAMPLER_ANISOTROPY_4,
};

struct ImageView {
  ImageRef img;
  uint32_t x_offset;
  uint32_t y_offset;
  uint32_t z_offset;
  uint32_t width;
  uint32_t height;
  uint32_t depth;
  ImageSampler sampler;
};

enum DepthImageUsageBits {
  L_DEPTH_IMAGE_USAGE_NONE = 0,
  L_DEPTH_IMAGE_USAGE_SAMPLED_BIT = (1 << 0),
  L_DEPTH_IMAGE_USAGE_ATTACHMENT_BIT = (1 << 1),
  L_DEPTH_IMAGE_USAGE_SUBPASS_DATA_BIT = (1 << 2),
  L_DEPTH_IMAGE_USAGE_TILE_MEMORY_BIT = (1 << 3),
};
typedef uint32_t DepthImageUsage;

enum DepthImageSampler {
  L_DEPTH_IMAGE_SAMPLER_LINEAR,
  L_DEPTH_IMAGE_SAMPLER_NEAREST,
  L_DEPTH_IMAGE_SAMPLER_ANISOTROPY_4,
};

struct DepthImageView {
  DepthImageRef depth_img;
  uint32_t x_offset;
  uint32_t y_offset;
  uint32_t width;
  uint32_t height;
  DepthImageSampler sampler;
};

enum ResourceType {
  L_RESOURCE_TYPE_UNIFORM_BUFFER,
  L_RESOURCE_TYPE_STORAGE_BUFFER,
  L_RESOURCE_TYPE_SAMPLED_IMAGE,
  L_RESOURCE_TYPE_STORAGE_IMAGE,
};

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

enum SubmitType {
  L_SUBMIT_TYPE_ANY,
  L_SUBMIT_TYPE_COMPUTE,
  L_SUBMIT_TYPE_GRAPHICS,
  L_SUBMIT_TYPE_TRANSFER,
  L_SUBMIT_TYPE_PRESENT,
};

struct DispatchSize {
  uint32_t x, y, z;
};

enum Topology {
  L_TOPOLOGY_POINT = 1,
  L_TOPOLOGY_LINE = 2,
  L_TOPOLOGY_TRIANGLE = 3,
  L_TOPOLOGY_TRIANGLE_WIREFRAME = 4,
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

// - [Create Configs] ----------------------------------------------------------

struct InstanceConfig {
  // Human-readable label of the instance.
  std::string label;
  // Application name.
  std::string app_name;
  // True to enable debug mode. More validation and logs.
  bool debug;
};

struct ContextConfig {
  // Human-readable label of the context.
  std::string label;
  // Index of the device.
  uint32_t device_index;
};
struct ContextWindowsConfig {
  std::string label;
  // Index of the device.
  uint32_t device_index;
  // Instance handle, aka `HINSTANCE`.
  const void* hinst;
  // Window handle, aka `HWND`.
  const void* hwnd;
};
struct ContextAndroidConfig {
  std::string label;
  // Index of the device.
  uint32_t device_index;
  // Android native window, `ANativeWindow`.
  const void* native_window;
};
struct ContextMetalConfig {
  std::string label;
  uint32_t device_index;
  const void* metal_layer;
};

// Describes a buffer.
struct BufferConfig {
  // Human-readable label of the buffer.
  std::string label;
  // Size of buffer allocation, or minimal size of buffer allocation if the
  // buffer has variable size. MUST NOT be zero.
  size_t size;
  // Host access pattern.
  MemoryAccess host_access;
  // Usage of the buffer.
  BufferUsage usage;
};

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
  fmt::Format format;
  // Color space of the image. Only linear and srgb are valid and it only
  // affects how the image data is interpreted on reads.
  fmt::ColorSpace color_space;
  // Usage of the image.
  ImageUsage usage;
};

struct DepthImageConfig {
  std::string label;
  // Width of the depth image. When used, the image size should match color
  // attachment size.
  uint32_t width;
  // Height of the depth image. When used, the image size should match color
  // attachment size.
  uint32_t height;
  // Pixel format of depth image.
  fmt::DepthFormat depth_format;
  // Usage of the depth image.
  DepthImageUsage usage;
};

struct SwapchainConfig {
  std::string label;
  // Number of image for multibuffering, can be 1, 2 or 3.
  uint32_t image_count;
  // Candidate image color formats. The format is selected based on platform
  // availability.
  std::vector<fmt::Format> allowed_formats;
  // Render output color space. Note that the color space is specified for
  // presentation. The rendering output should always be linear colors.
  fmt::ColorSpace color_space;
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
  std::vector<InvocationRef> invokes; // Lifetime bound.
  // Set `true` if the device-side execution time is wanted.
  bool is_timed;
};
struct CompositeInvocationConfig {
  std::string label;
  // Compute or render pass invocations within this composite invocation. Note
  // that graphics invocations cannot be called outside of render passes.
  std::vector<InvocationRef> invokes; // Lifetime bound.
  // Set `true` if the device-side execution time is wanted.
  bool is_timed;
};
struct PresentInvocationConfig {
};

} // namespace hal
} // namespace liong
