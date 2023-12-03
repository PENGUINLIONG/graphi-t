#include "gft/hal/hal.hpp"

namespace liong {
namespace hal {

// Please write me the config builders. Refers to the xxxConfig class
// definitions in src/gft/hal/hal.hpp for the config builder interfaces.

struct InstanceConfigBuilder {
  InstanceConfig inner {};

  InstanceConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }
  InstanceConfigBuilder& app_name(const std::string& app_name) {
    inner.app_name = app_name;
    return *this;
  }
  InstanceConfigBuilder& debug(bool debug) {
    inner.debug = debug;
    return *this;
  }

  operator InstanceConfig() const {
    return inner;
  }
};

struct ContextConfigBuilder {
  ContextConfig inner {};

  ContextConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }
  ContextConfigBuilder& device_index(uint32_t device_index) {
    inner.device_index = device_index;
    return *this;
  }

  operator ContextConfig() const {
    return inner;
  }
};

struct ContextWindowsConfigBuilder {
  ContextWindowsConfig inner {};

  ContextWindowsConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }
  ContextWindowsConfigBuilder& device_index(uint32_t device_index) {
    inner.device_index = device_index;
    return *this;
  }
  ContextWindowsConfigBuilder& hwnd(const void* hwnd) {
    inner.hwnd = hwnd;
    return *this;
  }
  ContextWindowsConfigBuilder& hinstance(const void* hinstance) {
    inner.hinst = hinstance;
    return *this;
  }

  operator ContextWindowsConfig() const {
    return inner;
  }
};

struct ContextAndroidConfigBuilder {
  ContextAndroidConfig inner {};

  ContextAndroidConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }
  ContextAndroidConfigBuilder& device_index(uint32_t device_index) {
    inner.device_index = device_index;
    return *this;
  }
  ContextAndroidConfigBuilder& native_window(const void* native_window) {
    inner.native_window = native_window;
    return *this;
  }

  operator ContextAndroidConfig() const {
    return inner;
  }
};

struct ContextMetalConfigBuilder {
  ContextMetalConfig inner {};

  ContextMetalConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }
  ContextMetalConfigBuilder& device_index(uint32_t device_index) {
    inner.device_index = device_index;
    return *this;
  }
  ContextMetalConfigBuilder& metal_layer(const void* metal_layer) {
    inner.metal_layer = metal_layer;
    return *this;
  }

  operator ContextMetalConfig() const {
    return inner;
  }
};

struct BufferConfigBuilder {
  BufferConfig inner {};

  BufferConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }
  BufferConfigBuilder& size(size_t size) {
    inner.size = size;
    return *this;
  }
  BufferConfigBuilder& host_access(MemoryAccess host_access) {
    inner.host_access = host_access;
    return *this;
  }

  BufferConfigBuilder& streaming() {
    return host_access(L_MEMORY_ACCESS_WRITE_BIT)
      .usage(L_BUFFER_USAGE_TRANSFER_SRC_BIT);
  }
  BufferConfigBuilder& read_back() {
    return host_access(L_MEMORY_ACCESS_READ_BIT)
      .usage(L_BUFFER_USAGE_TRANSFER_DST_BIT);
  }

  BufferConfigBuilder& usage(BufferUsage usage) {
    inner.usage |= usage;
    return *this;
  }
  BufferConfigBuilder& transfer() {
    return usage(
      L_BUFFER_USAGE_TRANSFER_SRC_BIT | L_BUFFER_USAGE_TRANSFER_DST_BIT
    );
  }
  BufferConfigBuilder& uniform() {
    return usage(L_BUFFER_USAGE_UNIFORM_BIT);
  }
  BufferConfigBuilder& storage() {
    return usage(L_BUFFER_USAGE_STORAGE_BIT);
  }
  BufferConfigBuilder& vertex() {
    return usage(L_BUFFER_USAGE_VERTEX_BIT);
  }
  BufferConfigBuilder& index() {
    return usage(L_BUFFER_USAGE_INDEX_BIT);
  }

  operator BufferConfig() const {
    return inner;
  }
};

struct ImageConfigBuilder {
  ImageConfig inner {};

  ImageConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }
  ImageConfigBuilder& width(uint32_t width) {
    inner.width = width;
    return *this;
  }
  ImageConfigBuilder& height(uint32_t height) {
    inner.height = height;
    return *this;
  }
  ImageConfigBuilder& depth(uint32_t depth) {
    inner.depth = depth;
    return *this;
  }
  ImageConfigBuilder& format(fmt::Format format) {
    inner.format = format;
    return *this;
  }
  ImageConfigBuilder& color_space(fmt::ColorSpace color_space) {
    inner.color_space = color_space;
    return *this;
  }
  ImageConfigBuilder& usage(ImageUsage usage) {
    inner.usage |= usage;
    return *this;
  }
  ImageConfigBuilder& transfer() {
    return usage(
      L_IMAGE_USAGE_TRANSFER_SRC_BIT | L_IMAGE_USAGE_TRANSFER_DST_BIT
    );
  }
  ImageConfigBuilder& sampled() {
    return usage(L_IMAGE_USAGE_SAMPLED_BIT);
  }
  ImageConfigBuilder& storage() {
    return usage(L_IMAGE_USAGE_STORAGE_BIT);
  }
  ImageConfigBuilder& attachment() {
    return usage(L_IMAGE_USAGE_ATTACHMENT_BIT);
  }
  ImageConfigBuilder& subpass_data() {
    return usage(L_IMAGE_USAGE_SUBPASS_DATA_BIT);
  }
  ImageConfigBuilder& tile_memory() {
    return usage(L_IMAGE_USAGE_TILE_MEMORY_BIT);
  }
  ImageConfigBuilder& present() {
    return usage(L_IMAGE_USAGE_PRESENT_BIT);
  }

  operator ImageConfig() const {
    return inner;
  }
};

struct DepthImageConfigBuilder {
  DepthImageConfig inner {};

  DepthImageConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }
  DepthImageConfigBuilder& width(uint32_t width) {
    inner.width = width;
    return *this;
  }
  DepthImageConfigBuilder& height(uint32_t height) {
    inner.height = height;
    return *this;
  }
  DepthImageConfigBuilder& depth_format(fmt::DepthFormat depth_format) {
    inner.depth_format = depth_format;
    return *this;
  }
  DepthImageConfigBuilder& usage(DepthImageUsage usage) {
    inner.usage |= usage;
    return *this;
  }
  DepthImageConfigBuilder& sampled() {
    return usage(L_DEPTH_IMAGE_USAGE_SAMPLED_BIT);
  }
  DepthImageConfigBuilder& attachment() {
    return usage(L_DEPTH_IMAGE_USAGE_ATTACHMENT_BIT);
  }
  DepthImageConfigBuilder& subpass_data() {
    return usage(L_DEPTH_IMAGE_USAGE_SUBPASS_DATA_BIT);
  }
  DepthImageConfigBuilder& tile_memory() {
    return usage(L_DEPTH_IMAGE_USAGE_TILE_MEMORY_BIT);
  }

  operator DepthImageConfig() const {
    return inner;
  }
};

struct SwapchainConfigBuilder {
  SwapchainConfig inner {};

  SwapchainConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }
  SwapchainConfigBuilder& image_count(uint32_t image_count) {
    inner.image_count = image_count;
    return *this;
  }
  SwapchainConfigBuilder& allowed_format(fmt::Format format) {
    inner.allowed_formats.push_back(format);
    return *this;
  }
  SwapchainConfigBuilder& color_space(fmt::ColorSpace color_space) {
    inner.color_space = color_space;
    return *this;
  }

  operator SwapchainConfig() const {
    return inner;
  }
};

struct ComputeTaskConfigBuilder {
  ComputeTaskConfig inner {};

  ComputeTaskConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }
  ComputeTaskConfigBuilder& comp_entry_name(const std::string& entry_name) {
    inner.entry_name = entry_name;
    return *this;
  }
  ComputeTaskConfigBuilder& compute_shader(
    const void* code,
    size_t size,
    const std::string& entry_point
  ) {
    inner.code = code;
    inner.code_size = size;
    inner.entry_name = entry_point;
    return *this;
  }
  ComputeTaskConfigBuilder& compute_shader(
    const std::string& code,
    const std::string& entry_point
  ) {
    return compute_shader(code.c_str(), code.size(), entry_point);
  }
  ComputeTaskConfigBuilder& compute_shader(
    const std::vector<uint8_t>& code,
    const std::string& entry_point
  ) {
    return compute_shader(code.data(), code.size(), entry_point);
  }
  ComputeTaskConfigBuilder& compute_shader(
    const std::vector<uint32_t>& code,
    const std::string& entry_point
  ) {
    return compute_shader(
      code.data(), code.size() * sizeof(uint32_t), entry_point
    );
  }
  ComputeTaskConfigBuilder& resource(ResourceType resource_type) {
    inner.rsc_tys.push_back(resource_type);
    return *this;
  }
  ComputeTaskConfigBuilder& uniform_buffer() {
    return resource(L_RESOURCE_TYPE_UNIFORM_BUFFER);
  }
  ComputeTaskConfigBuilder& storage_buffer() {
    return resource(L_RESOURCE_TYPE_STORAGE_BUFFER);
  }
  ComputeTaskConfigBuilder& sampled_image() {
    return resource(L_RESOURCE_TYPE_SAMPLED_IMAGE);
  }
  ComputeTaskConfigBuilder& storage_image() {
    return resource(L_RESOURCE_TYPE_STORAGE_IMAGE);
  }
  ComputeTaskConfigBuilder& workgrp_size(uint32_t x, uint32_t y, uint32_t z) {
    inner.workgrp_size.x = x;
    inner.workgrp_size.y = y;
    inner.workgrp_size.z = z;
    return *this;
  }
  ComputeTaskConfigBuilder& workgrp_size(uint32_t x, uint32_t y) {
    inner.workgrp_size.x = x;
    inner.workgrp_size.y = y;
    inner.workgrp_size.z = 1;
    return *this;
  }
  ComputeTaskConfigBuilder& workgrp_size(uint32_t x) {
    inner.workgrp_size.x = x;
    inner.workgrp_size.y = 1;
    inner.workgrp_size.z = 1;
    return *this;
  }
  ComputeTaskConfigBuilder& workgrp_size(const glm::uvec3& workgrp_size) {
    inner.workgrp_size.x = workgrp_size.x;
    inner.workgrp_size.y = workgrp_size.y;
    inner.workgrp_size.z = workgrp_size.z;
    return *this;
  }

  operator ComputeTaskConfig() const {
    return inner;
  }
};

struct GraphicsTaskConfigBuilder {
  GraphicsTaskConfig inner {};

  GraphicsTaskConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }
  GraphicsTaskConfigBuilder& vertex_shader(
    const void* code,
    size_t size,
    const std::string& entry_point
  ) {
    inner.vert_code = code;
    inner.vert_code_size = size;
    inner.vert_entry_name = entry_point;
    return *this;
  }
  GraphicsTaskConfigBuilder& vertex_shader(
    const std::string& code,
    const std::string& entry_point
  ) {
    return vertex_shader(code.c_str(), code.size(), entry_point);
  }
  GraphicsTaskConfigBuilder& vertex_shader(
    const std::vector<uint8_t>& code,
    const std::string& entry_point
  ) {
    return vertex_shader(code.data(), code.size(), entry_point);
  }
  GraphicsTaskConfigBuilder& vertex_shader(
    const std::vector<uint32_t>& code,
    const std::string& entry_point
  ) {
    return vertex_shader(
      code.data(), code.size() * sizeof(uint32_t), entry_point
    );
  }
  GraphicsTaskConfigBuilder& fragment_shader(
    const void* code,
    size_t size,
    const std::string& entry_point
  ) {
    inner.frag_code = code;
    inner.frag_code_size = size;
    inner.frag_entry_name = entry_point;
    return *this;
  }
  GraphicsTaskConfigBuilder& fragment_shader(
    const std::string& code,
    const std::string& entry_point
  ) {
    return fragment_shader(code.c_str(), code.size(), entry_point);
  }
  GraphicsTaskConfigBuilder& fragment_shader(
    const std::vector<uint8_t>& code,
    const std::string& entry_point
  ) {
    return fragment_shader(code.data(), code.size(), entry_point);
  }
  GraphicsTaskConfigBuilder& fragment_shader(
    const std::vector<uint32_t>& code,
    const std::string& entry_point
  ) {
    return fragment_shader(
      code.data(), code.size() * sizeof(uint32_t), entry_point
    );
  }
  GraphicsTaskConfigBuilder& topology(Topology topology) {
    inner.topo = topology;
    return *this;
  }
  GraphicsTaskConfigBuilder& resource(ResourceType resource_type) {
    inner.rsc_tys.push_back(resource_type);
    return *this;
  }
  GraphicsTaskConfigBuilder& uniform_buffer() {
    return resource(L_RESOURCE_TYPE_UNIFORM_BUFFER);
  }
  GraphicsTaskConfigBuilder& storage_buffer() {
    return resource(L_RESOURCE_TYPE_STORAGE_BUFFER);
  }
  GraphicsTaskConfigBuilder& sampled_image() {
    return resource(L_RESOURCE_TYPE_SAMPLED_IMAGE);
  }
  GraphicsTaskConfigBuilder& storage_image() {
    return resource(L_RESOURCE_TYPE_STORAGE_IMAGE);
  }

  operator GraphicsTaskConfig() const {
    return inner;
  }
};

struct AttachmentConfigBuilder {
  AttachmentConfig inner {};

  AttachmentConfigBuilder& attm_access(AttachmentAccess attm_access) {
    inner.attm_access = attm_access;
    return *this;
  }
  AttachmentConfigBuilder& color(
    fmt::Format format,
    fmt::ColorSpace color_space
  ) {
    inner.color_fmt = format;
    inner.cspace = color_space;
    return *this;
  }
  AttachmentConfigBuilder& depth(fmt::DepthFormat depth_format) {
    inner.depth_fmt = depth_format;
    return *this;
  }

  operator AttachmentConfig() const {
    return inner;
  }
};

struct RenderPassConfigBuilder {
  RenderPassConfig inner {};

  RenderPassConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }
  RenderPassConfigBuilder& width(uint32_t width) {
    inner.width = width;
    return *this;
  }
  RenderPassConfigBuilder& height(uint32_t height) {
    inner.height = height;
    return *this;
  }
  RenderPassConfigBuilder& color_attachment(
    AttachmentAccess attm_access,
    fmt::Format format,
    fmt::ColorSpace color_space
  ) {
    auto attachment = AttachmentConfigBuilder()
                        .attm_access(attm_access)
                        .color(format, color_space);
    inner.attm_cfgs.push_back(attachment);
    return *this;
  }
  RenderPassConfigBuilder& clear_store_color_attachment(
    fmt::Format format,
    fmt::ColorSpace color_space
  ) {
    return color_attachment(
      L_ATTACHMENT_ACCESS_CLEAR_BIT | L_ATTACHMENT_ACCESS_STORE_BIT,
      format,
      color_space
    );
  }
  RenderPassConfigBuilder& load_store_color_attachment(
    fmt::Format format,
    fmt::ColorSpace color_space
  ) {
    return color_attachment(
      L_ATTACHMENT_ACCESS_LOAD_BIT | L_ATTACHMENT_ACCESS_STORE_BIT,
      format,
      color_space
    );
  }
  RenderPassConfigBuilder& depth_attachment(
    AttachmentAccess attm_access,
    fmt::DepthFormat depth_format
  ) {
    auto attachment =
      AttachmentConfigBuilder().attm_access(attm_access).depth(depth_format);
    inner.attm_cfgs.push_back(attachment);
    return *this;
  }
  RenderPassConfigBuilder& clear_store_depth_attachment(
    fmt::DepthFormat depth_format
  ) {
    return depth_attachment(
      L_ATTACHMENT_ACCESS_CLEAR_BIT | L_ATTACHMENT_ACCESS_STORE_BIT,
      depth_format
    );
  }
  RenderPassConfigBuilder& load_store_depth_attachment(
    fmt::DepthFormat depth_format
  ) {
    return depth_attachment(
      L_ATTACHMENT_ACCESS_LOAD_BIT | L_ATTACHMENT_ACCESS_STORE_BIT, depth_format
    );
  }

  operator RenderPassConfig() const {
    return inner;
  }
};

struct TransferInvocationConfigBuilder {
  TransferInvocationConfig inner {};

  TransferInvocationConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }
  TransferInvocationConfigBuilder& src(BufferView src) {
    inner.src_rsc_view.rsc_view_ty = L_RESOURCE_VIEW_TYPE_BUFFER;
    inner.src_rsc_view.buf_view = src;
    return *this;
  }
  TransferInvocationConfigBuilder& src(ImageView src) {
    inner.src_rsc_view.rsc_view_ty = L_RESOURCE_VIEW_TYPE_IMAGE;
    inner.src_rsc_view.img_view = src;
    return *this;
  }
  TransferInvocationConfigBuilder& dst(BufferView dst) {
    inner.dst_rsc_view.rsc_view_ty = L_RESOURCE_VIEW_TYPE_BUFFER;
    inner.dst_rsc_view.buf_view = dst;
    return *this;
  }
  TransferInvocationConfigBuilder& dst(ImageView dst) {
    inner.dst_rsc_view.rsc_view_ty = L_RESOURCE_VIEW_TYPE_IMAGE;
    inner.dst_rsc_view.img_view = dst;
    return *this;
  }
  TransferInvocationConfigBuilder& is_timed(bool timed) {
    inner.is_timed = timed;
    return *this;
  }

  operator TransferInvocationConfig() const {
    return inner;
  }
};

struct ComputeInvocationConfigBuilder {
  ComputeInvocationConfig inner {};

  ComputeInvocationConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }

  ComputeInvocationConfigBuilder& resource(const BufferView& resource) {
    inner.rsc_views.push_back(ResourceView::buffer(resource));
    return *this;
  }
  ComputeInvocationConfigBuilder& resource(const ImageView& resource) {
    inner.rsc_views.push_back(ResourceView::image(resource));
    return *this;
  }
  ComputeInvocationConfigBuilder& resource(const DepthImageView& resource) {
    inner.rsc_views.push_back(ResourceView::depth_image(resource));
    return *this;
  }
  ComputeInvocationConfigBuilder& workgroup_count(
    uint32_t x,
    uint32_t y,
    uint32_t z
  ) {
    inner.workgrp_count.x = x;
    inner.workgrp_count.y = y;
    inner.workgrp_count.z = z;
    return *this;
  }
  ComputeInvocationConfigBuilder& workgroup_count(uint32_t x, uint32_t y) {
    inner.workgrp_count.x = x;
    inner.workgrp_count.y = y;
    inner.workgrp_count.z = 1;
    return *this;
  }
  ComputeInvocationConfigBuilder& workgroup_count(uint32_t x) {
    inner.workgrp_count.x = x;
    inner.workgrp_count.y = 1;
    inner.workgrp_count.z = 1;
    return *this;
  }
  ComputeInvocationConfigBuilder& workgroup_count(
    const glm::uvec3& workgroup_count
  ) {
    inner.workgrp_count.x = workgroup_count.x;
    inner.workgrp_count.y = workgroup_count.y;
    inner.workgrp_count.z = workgroup_count.z;
    return *this;
  }
  ComputeInvocationConfigBuilder& is_timed(bool timed) {
    inner.is_timed = timed;
    return *this;
  }

  operator ComputeInvocationConfig() const {
    return inner;
  }
};

struct GraphicsInvocationConfigBuilder {
  GraphicsInvocationConfig inner {};

  GraphicsInvocationConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }

  GraphicsInvocationConfigBuilder& resource(const BufferView& resource) {
    inner.rsc_views.push_back(ResourceView::buffer(resource));
    return *this;
  }
  GraphicsInvocationConfigBuilder& resource(const ImageView& resource) {
    inner.rsc_views.push_back(ResourceView::image(resource));
    return *this;
  }
  GraphicsInvocationConfigBuilder& resource(const DepthImageView& resource) {
    inner.rsc_views.push_back(ResourceView::depth_image(resource));
    return *this;
  }
  GraphicsInvocationConfigBuilder& vertex_buffer(const BufferView& vertex_buffer
  ) {
    inner.vert_bufs.push_back(vertex_buffer);
    return *this;
  }
  GraphicsInvocationConfigBuilder& per_index(
    const BufferView& index_buffer,
    uint32_t index_count,
    IndexType index_type,
    uint32_t instance_count
  ) {
    inner.idx_buf = index_buffer;
    inner.nvert = index_count;
    inner.idx_ty = index_type;
    inner.ninst = instance_count;
    return *this;
  }
  GraphicsInvocationConfigBuilder& per_index(
    const BufferView& index_buffer,
    uint32_t index_count,
    IndexType index_type
  ) {
    return per_index(index_buffer, index_count, index_type, 1);
  }
  GraphicsInvocationConfigBuilder& per_u32_index(
    const BufferView& index_buffer,
    uint32_t index_count,
    uint32_t instance_count
  ) {
    return per_index(
      index_buffer, index_count, L_INDEX_TYPE_UINT32, instance_count
    );
  }
  GraphicsInvocationConfigBuilder& per_u32_index(
    const BufferView& index_buffer,
    uint32_t index_count
  ) {
    return per_index(index_buffer, index_count, L_INDEX_TYPE_UINT32, 1);
  }
  GraphicsInvocationConfigBuilder& per_u16_index(
    const BufferView& index_buffer,
    uint32_t index_count,
    uint32_t instance_count
  ) {
    return per_index(
      index_buffer, index_count, L_INDEX_TYPE_UINT16, instance_count
    );
  }
  GraphicsInvocationConfigBuilder& per_u16_index(
    const BufferView& index_buffer,
    uint32_t index_count
  ) {
    return per_index(index_buffer, index_count, L_INDEX_TYPE_UINT16, 1);
  }
  GraphicsInvocationConfigBuilder& per_vertex(
    uint32_t vertex_count,
    uint32_t instance_count
  ) {
    inner.nvert = vertex_count;
    inner.ninst = instance_count;
    return *this;
  }
  GraphicsInvocationConfigBuilder& per_vertex(uint32_t vertex_count) {
    return per_vertex(vertex_count, 1);
  }
  GraphicsInvocationConfigBuilder& is_timed(bool timed) {
    inner.is_timed = timed;
    return *this;
  }

  operator GraphicsInvocationConfig() const {
    return inner;
  }
};

struct RenderPassInvocationConfigBuilder {
  RenderPassInvocationConfig inner {};

  RenderPassInvocationConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }
  RenderPassInvocationConfigBuilder& attachment(ImageView attachment) {
    inner.attms.push_back(ResourceView::image(attachment));
    return *this;
  }
  RenderPassInvocationConfigBuilder& attachment(DepthImageView attachment) {
    inner.attms.push_back(ResourceView::depth_image(attachment));
    return *this;
  }
  RenderPassInvocationConfigBuilder& invocation(const InvocationRef& invocation
  ) {
    inner.invokes.push_back(invocation);
    return *this;
  }
  RenderPassInvocationConfigBuilder& is_timed(bool timed) {
    inner.is_timed = timed;
    return *this;
  }

  operator RenderPassInvocationConfig() const {
    return inner;
  }
};

struct CompositeInvocationConfigBuilder {
  CompositeInvocationConfig inner {};

  CompositeInvocationConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }
  CompositeInvocationConfigBuilder& invocation(const InvocationRef& invocation
  ) {
    inner.invokes.push_back(invocation);
    return *this;
  }
  CompositeInvocationConfigBuilder& invocations(const std::vector<InvocationRef>& invocations
  ) {
    inner.invokes.insert(
      inner.invokes.end(), invocations.begin(), invocations.end()
    );
    return *this;
  }
  CompositeInvocationConfigBuilder& is_timed(bool timed) {
    inner.is_timed = timed;
    return *this;
  }

  operator CompositeInvocationConfig() const {
    return inner;
  }
};

struct PresentInvocationConfigBuilder {
  PresentInvocationConfig inner {};

  PresentInvocationConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }

  operator PresentInvocationConfig() const {
    return inner;
  }
};

struct TransactionConfigBuilder {
  TransactionConfig inner {};

  TransactionConfigBuilder& label(const std::string& label) {
    inner.label = label;
    return *this;
  }

  operator TransactionConfig() const {
    return inner;
  }
};

} // namespace hal
} // namespace liong
