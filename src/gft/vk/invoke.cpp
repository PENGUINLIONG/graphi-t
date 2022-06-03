#include "gft/vk.hpp"
#include "gft/log.hpp"

namespace liong {
namespace vk {

VkDescriptorPool _create_desc_pool(
  const Context& ctxt,
  const std::vector<VkDescriptorPoolSize>& desc_pool_sizes
) {
  VkDescriptorPoolCreateInfo dpci {};
  dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  dpci.poolSizeCount = static_cast<uint32_t>(desc_pool_sizes.size());
  dpci.pPoolSizes = desc_pool_sizes.data();
  dpci.maxSets = 1;

  VkDescriptorPool desc_pool;
  VK_ASSERT << vkCreateDescriptorPool(ctxt.dev, &dpci, nullptr, &desc_pool);
  return desc_pool;
}
VkDescriptorSet _alloc_desc_set(
  const Context& ctxt,
  VkDescriptorPool desc_pool,
  VkDescriptorSetLayout desc_set_layout
) {
  VkDescriptorSetAllocateInfo dsai {};
  dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dsai.descriptorPool = desc_pool;
  dsai.descriptorSetCount = 1;
  dsai.pSetLayouts = &desc_set_layout;

  VkDescriptorSet desc_set;
  VK_ASSERT << vkAllocateDescriptorSets(ctxt.dev, &dsai, &desc_set);
  return desc_set;
}
void _update_desc_set(
  const Context& ctxt,
  VkDescriptorSet desc_set,
  const std::vector<ResourceType>& rsc_tys,
  const std::vector<ResourceView>& rsc_views
) {
  std::vector<VkDescriptorBufferInfo> dbis;
  std::vector<VkDescriptorImageInfo> diis;
  std::vector<VkWriteDescriptorSet> wdss;
  dbis.reserve(rsc_views.size());
  diis.reserve(rsc_views.size());
  wdss.reserve(rsc_views.size());

  auto push_dbi = [&](const ResourceView& rsc_view) {
    L_ASSERT(rsc_view.rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER);
    const BufferView& buf_view = rsc_view.buf_view;

    VkDescriptorBufferInfo dbi {};
    dbi.buffer = buf_view.buf->buf;
    dbi.offset = buf_view.offset;
    dbi.range = buf_view.size;
    dbis.emplace_back(std::move(dbi));

    log::debug("bound pool resource #", wdss.size(), " to buffer '",
      buf_view.buf->buf_cfg.label, "'");

    return &dbis.back();
  };
  auto push_dii = [&](const ResourceView& rsc_view, VkImageLayout layout) {
    VkDescriptorImageInfo dii {};
    if (rsc_view.rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE) {
      const ImageView& img_view = rsc_view.img_view;
      dii.sampler = ctxt.img_samplers.at(img_view.sampler);
      dii.imageView = img_view.img->img_view;
      dii.imageLayout = layout;
      diis.emplace_back(std::move(dii));

      log::debug("bound pool resource #", wdss.size(), " to image '",
        img_view.img->img_cfg.label, "'");
    } else if (rsc_view.rsc_view_ty == L_RESOURCE_VIEW_TYPE_DEPTH_IMAGE) {
      const DepthImageView& depth_img_view = rsc_view.depth_img_view;

      dii.sampler = ctxt.depth_img_samplers.at(depth_img_view.sampler);
      dii.imageView = depth_img_view.depth_img->img_view;
      dii.imageLayout = layout;
      diis.emplace_back(std::move(dii));

      log::debug("bound pool resource #", wdss.size(), " to depth image '",
        depth_img_view.depth_img->depth_img_cfg.label, "'");
    } else {
      panic();
    }

    return &diis.back();
  };

  for (uint32_t i = 0; i < rsc_views.size(); ++i) {
    const ResourceView& rsc_view = rsc_views[i];

    VkWriteDescriptorSet wds {};
    wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    wds.dstSet = desc_set;
    wds.dstBinding = i;
    wds.dstArrayElement = 0;
    wds.descriptorCount = 1;
    switch (rsc_tys[i]) {
    case L_RESOURCE_TYPE_UNIFORM_BUFFER:
      wds.pBufferInfo = push_dbi(rsc_view);
      wds.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      break;
    case L_RESOURCE_TYPE_STORAGE_BUFFER:
      wds.pBufferInfo = push_dbi(rsc_view);
      wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      break;
    case L_RESOURCE_TYPE_SAMPLED_IMAGE:
      wds.pImageInfo = push_dii(rsc_view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
      wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      break;
    case L_RESOURCE_TYPE_STORAGE_IMAGE:
      wds.pImageInfo = push_dii(rsc_view,
        VK_IMAGE_LAYOUT_GENERAL);
      wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      break;
    default: panic("unexpected resource type");
    }
    wdss.emplace_back(std::move(wds));
  }

  vkUpdateDescriptorSets(ctxt.dev, (uint32_t)wdss.size(), wdss.data(), 0,
    nullptr);
}
VkFramebuffer _create_framebuf(
  const RenderPass& pass,
  const std::vector<ResourceView>& attms
) {
  const RenderPassConfig& pass_cfg = pass.pass_cfg;

  L_ASSERT(pass_cfg.attm_cfgs.size() == attms.size(),
    "number of provided attachments mismatches render pass requirement");
  std::vector<VkImageView> attm_img_views;

  uint32_t width = pass_cfg.width;
  uint32_t height = pass_cfg.height;

  for (size_t i = 0; i < attms.size(); ++i) {
    const AttachmentConfig& attm_cfg = pass_cfg.attm_cfgs[i];
    const ResourceView& attm = attms[i];

    switch (attm_cfg.attm_ty) {
    case L_ATTACHMENT_TYPE_COLOR:
    {
      const Image& img = *attm.img_view.img;
      const ImageConfig& img_cfg = img.img_cfg;
      L_ASSERT(attm.rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE);
      L_ASSERT(img_cfg.width == width && img_cfg.height == height,
        "color attachment size mismatches framebuffer size");
      attm_img_views.emplace_back(img.img_view);
      break;
    }
    case L_ATTACHMENT_TYPE_DEPTH:
    {
      const DepthImage& depth_img = *attm.depth_img_view.depth_img;
      const DepthImageConfig& depth_img_cfg = depth_img.depth_img_cfg;
      L_ASSERT(attm.rsc_view_ty == L_RESOURCE_VIEW_TYPE_DEPTH_IMAGE);
      L_ASSERT(depth_img_cfg.width == width && depth_img_cfg.height == height,
        "depth attachment size mismatches framebuffer size");
      attm_img_views.emplace_back(depth_img.img_view);
      break;
    }
    default: panic("unexpected attachment type");
    }
  }

  VkFramebufferCreateInfo fci {};
  fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fci.renderPass = pass.pass;
  fci.attachmentCount = (uint32_t)attm_img_views.size();
  fci.pAttachments = attm_img_views.data();
  fci.width = width;
  fci.height = height;
  fci.layers = 1;

  VkFramebuffer framebuf;
  VK_ASSERT << vkCreateFramebuffer(pass.ctxt->dev, &fci, nullptr, &framebuf);

  return framebuf;
}
VkQueryPool _create_query_pool(
  const Context& ctxt,
  VkQueryType query_ty,
  uint32_t nquery
) {
  VkQueryPoolCreateInfo qpci {};
  qpci.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  qpci.queryType = query_ty;
  qpci.queryCount = nquery;

  VkQueryPool query_pool;
  VK_ASSERT << vkCreateQueryPool(ctxt.dev, &qpci, nullptr, &query_pool);

  return query_pool;
}
void _collect_task_invoke_transit(
  const std::vector<ResourceView> rsc_views,
  const std::vector<ResourceType> rsc_tys,
  InvocationTransitionDetail& transit_detail
) {
  L_ASSERT(rsc_views.size() == rsc_tys.size());

  auto& buf_transit = transit_detail.buf_transit;
  auto& img_transit = transit_detail.img_transit;
  auto& depth_img_transit = transit_detail.depth_img_transit;

  for (size_t i = 0; i < rsc_views.size(); ++i) {
    const ResourceView& rsc_view = rsc_views[i];
    ResourceViewType rsc_view_ty = rsc_view.rsc_view_ty;
    ResourceType rsc_ty = rsc_tys[i];

    switch (rsc_ty) {
    case L_RESOURCE_TYPE_UNIFORM_BUFFER:
      if (rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER) {
        transit_detail.reg(rsc_view.buf_view, L_BUFFER_USAGE_UNIFORM_BIT);
      } else {
        unreachable();
      }
      break;
    case L_RESOURCE_TYPE_STORAGE_BUFFER:
      if (rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER) {
        transit_detail.reg(rsc_view.buf_view, L_BUFFER_USAGE_STORAGE_BIT);
      } else {
        unreachable();
      }
      break;
    case L_RESOURCE_TYPE_SAMPLED_IMAGE:
      if (rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE) {
        transit_detail.reg(rsc_view.img_view, L_IMAGE_USAGE_SAMPLED_BIT);
      } else if (rsc_view_ty == L_RESOURCE_VIEW_TYPE_DEPTH_IMAGE) {
        transit_detail.reg(rsc_view.depth_img_view,
          L_DEPTH_IMAGE_USAGE_SAMPLED_BIT);
      } else {
        unreachable();
      }
      break;
    case L_RESOURCE_TYPE_STORAGE_IMAGE:
      if (rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE) {
        transit_detail.reg(rsc_view.img_view, L_IMAGE_USAGE_STORAGE_BIT);
      } else {
        unreachable();
      }
      break;
    default: unreachable();
    }
  }
}
void _merge_subinvoke_transits(
  const std::vector<const Invocation*>& subinvokes,
  InvocationTransitionDetail& transit_detail
) {
  for (const auto& subinvoke : subinvokes) {
    L_ASSERT(subinvoke != nullptr);
    for (const auto& pair : subinvoke->transit_detail.buf_transit) {
      transit_detail.buf_transit.emplace_back(pair);
    }
    for (const auto& pair : subinvoke->transit_detail.img_transit) {
      transit_detail.img_transit.emplace_back(pair);
    }
    for (const auto& pair : subinvoke->transit_detail.depth_img_transit) {
      transit_detail.depth_img_transit.emplace_back(pair);
    }
  }
}
void _merge_subinvoke_transits(
  const Invocation* subinvoke,
  InvocationTransitionDetail& transit_detail
) {
  std::vector<const Invocation*> subinvokes { subinvoke };
  _merge_subinvoke_transits(subinvokes, transit_detail);
}
SubmitType _infer_submit_ty(const std::vector<const Invocation*>& subinvokes) {
  for (size_t i = 0; i < subinvokes.size(); ++i) {
    SubmitType submit_ty = subinvokes[i]->submit_ty;
    if (submit_ty != L_SUBMIT_TYPE_ANY) { return submit_ty; }
  }
  return L_SUBMIT_TYPE_ANY;
}
VkBufferCopy _make_bc(const BufferView& src, const BufferView& dst) {
  VkBufferCopy bc {};
  bc.srcOffset = src.offset;
  bc.dstOffset = dst.offset;
  bc.size = dst.size;
  return bc;
}
VkImageCopy _make_ic(const ImageView& src, const ImageView& dst) {
  VkImageCopy ic {};
  ic.srcOffset.x = src.x_offset;
  ic.srcOffset.y = src.y_offset;
  ic.dstOffset.x = dst.x_offset;
  ic.dstOffset.y = dst.y_offset;
  ic.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  ic.srcSubresource.baseArrayLayer = 0;
  ic.srcSubresource.layerCount = 1;
  ic.srcSubresource.mipLevel = 0;
  ic.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  ic.dstSubresource.baseArrayLayer = 0;
  ic.dstSubresource.layerCount = 1;
  ic.dstSubresource.mipLevel = 0;
  ic.extent.width = dst.width;
  ic.extent.height = dst.height == 0 ? 1 : dst.height;
  ic.extent.depth = dst.depth == 0 ? 1 : dst.depth;
  return ic;
}
VkBufferImageCopy _make_bic(const BufferView& buf, const ImageView& img) {
  VkBufferImageCopy bic {};
  bic.bufferOffset = buf.offset;
  bic.bufferRowLength = 0;
  bic.bufferImageHeight = (uint32_t)img.img->img_cfg.height;
  bic.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bic.imageSubresource.mipLevel = 0;
  bic.imageSubresource.baseArrayLayer = 0;
  bic.imageSubresource.layerCount = 1;
  bic.imageOffset.x = img.x_offset;
  bic.imageOffset.y = img.y_offset;
  bic.imageExtent.width = img.width;
  bic.imageExtent.height = img.height == 0 ? 1 : img.height;
  bic.imageExtent.depth = img.depth == 0 ? 1 : img.depth;
  return bic;
}
void _fill_transfer_b2b_invoke(
  const BufferView& src,
  const BufferView& dst,
  Invocation& out
) {
  InvocationCopyBufferToBufferDetail b2b_detail {};
  b2b_detail.bc = _make_bc(src, dst);
  b2b_detail.src = src.buf->buf;
  b2b_detail.dst = dst.buf->buf;

  out.b2b_detail =
    std::make_unique<InvocationCopyBufferToBufferDetail>(std::move(b2b_detail));

  out.transit_detail.reg(src, L_BUFFER_USAGE_TRANSFER_SRC_BIT);
  out.transit_detail.reg(dst, L_BUFFER_USAGE_TRANSFER_DST_BIT);
}
void _fill_transfer_b2i_invoke(
  const BufferView& src,
  const ImageView& dst,
  Invocation& out
) {
  InvocationCopyBufferToImageDetail b2i_detail {};
  b2i_detail.bic = _make_bic(src, dst);
  b2i_detail.src = src.buf->buf;
  b2i_detail.dst = dst.img->img;

  out.b2i_detail =
    std::make_unique<InvocationCopyBufferToImageDetail>(std::move(b2i_detail));

  out.transit_detail.reg(src, L_BUFFER_USAGE_TRANSFER_SRC_BIT);
  out.transit_detail.reg(dst, L_IMAGE_USAGE_TRANSFER_DST_BIT);
}
void _fill_transfer_i2b_invoke(
  const ImageView& src,
  const BufferView& dst,
  Invocation& out
) {
  InvocationCopyImageToBufferDetail i2b_detail {};
  i2b_detail.bic = _make_bic(dst, src);
  i2b_detail.src = src.img->img;
  i2b_detail.dst = dst.buf->buf;

  out.i2b_detail =
    std::make_unique<InvocationCopyImageToBufferDetail>(std::move(i2b_detail));

  out.transit_detail.reg(src, L_IMAGE_USAGE_TRANSFER_SRC_BIT);
  out.transit_detail.reg(dst, L_BUFFER_USAGE_TRANSFER_DST_BIT);
}
void _fill_transfer_i2i_invoke(
  const ImageView& src,
  const ImageView& dst,
  Invocation& out
) {
  InvocationCopyImageToImageDetail i2i_detail {};
  i2i_detail.ic = _make_ic(src, dst);
  i2i_detail.src = src.img->img;
  i2i_detail.dst = dst.img->img;

  out.i2i_detail =
    std::make_unique<InvocationCopyImageToImageDetail>(std::move(i2i_detail));

  out.transit_detail.reg(src, L_IMAGE_USAGE_TRANSFER_SRC_BIT);
  out.transit_detail.reg(dst, L_IMAGE_USAGE_TRANSFER_DST_BIT);
}
Invocation create_trans_invoke(
  const Context& ctxt,
  const TransferInvocationConfig& cfg
) {
  const ResourceView& src_rsc_view = cfg.src_rsc_view;
  const ResourceView& dst_rsc_view = cfg.dst_rsc_view;
  ResourceViewType src_rsc_view_ty = src_rsc_view.rsc_view_ty;
  ResourceViewType dst_rsc_view_ty = dst_rsc_view.rsc_view_ty;

  Invocation out {};
  out.label = cfg.label;
  out.ctxt = &ctxt;
  out.submit_ty = L_SUBMIT_TYPE_TRANSFER;
  out.query_pool = cfg.is_timed ?
    _create_query_pool(ctxt, VK_QUERY_TYPE_TIMESTAMP, 2) : VK_NULL_HANDLE;

  if (
    src_rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER &&
    dst_rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER
  ) {
    _fill_transfer_b2b_invoke(
      src_rsc_view.buf_view, dst_rsc_view.buf_view, out);
  } else if (
    src_rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER &&
    dst_rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE
  ) {
    _fill_transfer_b2i_invoke(
      src_rsc_view.buf_view, dst_rsc_view.img_view, out);
  } else if (
    src_rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE &&
    dst_rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER
  ) {
    _fill_transfer_i2b_invoke(
      src_rsc_view.img_view, dst_rsc_view.buf_view, out);
  } else if (
    src_rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE &&
    dst_rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE
  ) {
    _fill_transfer_i2i_invoke(
      src_rsc_view.img_view, dst_rsc_view.img_view, out);
  } else {
    panic("depth image cannot be transferred");
  }

  log::debug("created transfer invocation");
  return out;
}
Invocation create_comp_invoke(
  const Task& task,
  const ComputeInvocationConfig& cfg
) {
  L_ASSERT(task.rsc_tys.size() == cfg.rsc_views.size());
  L_ASSERT(task.submit_ty == L_SUBMIT_TYPE_COMPUTE);
  const Context& ctxt = *task.ctxt;

  Invocation out {};
  out.label = cfg.label;
  out.ctxt = &ctxt;
  out.submit_ty = L_SUBMIT_TYPE_COMPUTE;
  out.query_pool = cfg.is_timed ?
    _create_query_pool(ctxt, VK_QUERY_TYPE_TIMESTAMP, 2) : VK_NULL_HANDLE;

  InvocationTransitionDetail transit_detail {};
  _collect_task_invoke_transit(cfg.rsc_views, task.rsc_tys, transit_detail);
  out.transit_detail = std::move(transit_detail);

  InvocationComputeDetail comp_detail {};
  comp_detail.task = &task;
  comp_detail.bind_pt = VK_PIPELINE_BIND_POINT_COMPUTE;
  if (task.desc_pool_sizes.size() > 0) {
    comp_detail.desc_pool = _create_desc_pool(ctxt, task.desc_pool_sizes);
    comp_detail.desc_set =
      _alloc_desc_set(ctxt, comp_detail.desc_pool, task.desc_set_layout);
    _update_desc_set(ctxt, comp_detail.desc_set, task.rsc_tys, cfg.rsc_views);
  }
  comp_detail.workgrp_count = cfg.workgrp_count;

  out.comp_detail =
    std::make_unique<InvocationComputeDetail>(std::move(comp_detail));

  log::debug("created compute invocation");
  return out;
}
Invocation create_graph_invoke(
  const Task& task,
  const GraphicsInvocationConfig& cfg
) {
  L_ASSERT(task.rsc_tys.size() == cfg.rsc_views.size());
  L_ASSERT(task.submit_ty == L_SUBMIT_TYPE_GRAPHICS);
  const Context& ctxt = *task.ctxt;

  Invocation out {};
  out.label = cfg.label;
  out.ctxt = &ctxt;
  out.submit_ty = L_SUBMIT_TYPE_GRAPHICS;
  out.query_pool = cfg.is_timed ?
    _create_query_pool(ctxt, VK_QUERY_TYPE_TIMESTAMP, 2) : VK_NULL_HANDLE;

  InvocationTransitionDetail transit_detail {};
  _collect_task_invoke_transit(cfg.rsc_views, task.rsc_tys, transit_detail);
  for (size_t i = 0; i < cfg.vert_bufs.size(); ++i) {
    transit_detail.reg(cfg.vert_bufs[i], L_BUFFER_USAGE_VERTEX_BIT);
  }
  if (cfg.nidx > 0) {
    transit_detail.reg(cfg.idx_buf, L_BUFFER_USAGE_INDEX_BIT);
  }
  out.transit_detail = std::move(transit_detail);

  std::vector<VkBuffer> vert_bufs;
  std::vector<VkDeviceSize> vert_buf_offsets;
  vert_bufs.reserve(cfg.vert_bufs.size());
  vert_buf_offsets.reserve(cfg.vert_bufs.size());
  for (size_t i = 0; i < cfg.vert_bufs.size(); ++i) {
    const BufferView& vert_buf = cfg.vert_bufs[i];
    vert_bufs.emplace_back(vert_buf.buf->buf);
    vert_buf_offsets.emplace_back(vert_buf.offset);
  }

  InvocationGraphicsDetail graph_detail {};
  graph_detail.task = &task;
  graph_detail.bind_pt = VK_PIPELINE_BIND_POINT_GRAPHICS;
  if (task.desc_pool_sizes.size() > 0) {
    graph_detail.desc_pool = _create_desc_pool(ctxt, task.desc_pool_sizes);
    graph_detail.desc_set =
      _alloc_desc_set(ctxt, graph_detail.desc_pool, task.desc_set_layout);
    _update_desc_set(ctxt, graph_detail.desc_set, task.rsc_tys, cfg.rsc_views);
  }
  graph_detail.vert_bufs = std::move(vert_bufs);
  graph_detail.vert_buf_offsets = std::move(vert_buf_offsets);
  graph_detail.idx_buf = cfg.idx_buf.buf->buf;
  graph_detail.idx_buf_offset = cfg.idx_buf.offset;
  graph_detail.ninst = cfg.ninst;
  graph_detail.nvert = cfg.nvert;
  graph_detail.idx_ty = cfg.idx_ty;
  graph_detail.nidx = cfg.nidx;

  out.graph_detail =
    std::make_unique<InvocationGraphicsDetail>(std::move(graph_detail));

  log::debug("created graphics invocation");
  return out;
}
Invocation create_pass_invoke(
  const RenderPass& pass,
  const RenderPassInvocationConfig& cfg
) {
  const Context& ctxt = *pass.ctxt;

  Invocation out {};
  out.label = cfg.label;
  out.ctxt = &ctxt;
  out.submit_ty = L_SUBMIT_TYPE_GRAPHICS;
  out.query_pool = cfg.is_timed ?
    _create_query_pool(ctxt, VK_QUERY_TYPE_TIMESTAMP, 2) : VK_NULL_HANDLE;

  InvocationTransitionDetail transit_detail {};
  for (size_t i = 0; i < cfg.attms.size(); ++i) {
    const ResourceView& attm = cfg.attms[i];

    switch (attm.rsc_view_ty) {
    case L_RESOURCE_VIEW_TYPE_IMAGE:
      transit_detail.reg(attm.img_view, L_IMAGE_USAGE_ATTACHMENT_BIT);
      break;
    case L_RESOURCE_VIEW_TYPE_DEPTH_IMAGE:
      transit_detail.reg(attm.depth_img_view,
        L_DEPTH_IMAGE_USAGE_ATTACHMENT_BIT);
      break;
    default: panic("render pass attachment must be image or depth image");
    }
  }
  _merge_subinvoke_transits(cfg.invokes, transit_detail);
  out.transit_detail = std::move(transit_detail);

  InvocationRenderPassDetail pass_detail {};
  pass_detail.pass = &pass;
  pass_detail.framebuf = _create_framebuf(pass, cfg.attms);
  // TODO: (penguinliong) Command buffer baking.
  pass_detail.is_baked = false;
  pass_detail.subinvokes = cfg.invokes;

  for (size_t i = 0; i < cfg.invokes.size(); ++i) {
    const Invocation& invoke = *cfg.invokes[i];
    L_ASSERT(invoke.graph_detail != nullptr,
      "render pass invocation constituent must be graphics task invocation");
  }

  out.pass_detail =
    std::make_unique<InvocationRenderPassDetail>(std::move(pass_detail));

  log::debug("created render pass invocation");
  return out;
}
Invocation create_present_invoke(const Swapchain& swapchain) {
  L_ASSERT(swapchain.dyn_detail != nullptr,
    "swapchain need to be recreated with `acquire_swapchain_img`");

  const Context& ctxt = *swapchain.ctxt;
  SwapchainDynamicDetail& dyn_detail =
    (SwapchainDynamicDetail&)*swapchain.dyn_detail;

  L_ASSERT(dyn_detail.img_idx != nullptr,
    "swapchain has not acquired an image to present for the current frame");

  Invocation out {};
  out.label = util::format(swapchain.swapchain_cfg.label);
  out.ctxt = &ctxt;
  out.submit_ty = L_SUBMIT_TYPE_PRESENT;
  out.query_pool = VK_NULL_HANDLE;

  InvocationPresentDetail present_detail {};
  present_detail.swapchain = &swapchain;

  out.present_detail =
    std::make_unique<InvocationPresentDetail>(std::move(present_detail));

  log::debug("created present invocation");
  return out;
}
Invocation create_composite_invoke(
  const Context& ctxt,
  const CompositeInvocationConfig& cfg
) {
  L_ASSERT(cfg.invokes.size() > 0);

  Invocation out {};
  out.label = cfg.label;
  out.ctxt = &ctxt;
  out.submit_ty = _infer_submit_ty(cfg.invokes);
  out.query_pool = cfg.is_timed ?
    _create_query_pool(ctxt, VK_QUERY_TYPE_TIMESTAMP, 2) : VK_NULL_HANDLE;

  InvocationTransitionDetail transit_detail {};
  _merge_subinvoke_transits(cfg.invokes, transit_detail);
  out.transit_detail = std::move(transit_detail);

  InvocationCompositeDetail composite_detail {};
  composite_detail.subinvokes = cfg.invokes;

  out.composite_detail =
    std::make_unique<InvocationCompositeDetail>(std::move(composite_detail));

  log::debug("created composition invocation");
  return out;
}
void destroy_invoke(Invocation& invoke) {
  const Context& ctxt = *invoke.ctxt;
  if (
    invoke.b2b_detail ||
    invoke.b2i_detail ||
    invoke.i2b_detail ||
    invoke.i2i_detail
  ) {
    log::debug("destroyed transfer invocation '", invoke.label, "'");
  }
  if (invoke.comp_detail) {
    const InvocationComputeDetail& comp_detail = *invoke.comp_detail;
    vkDestroyDescriptorPool(ctxt.dev, comp_detail.desc_pool, nullptr);
    log::debug("destroyed compute invocation '", invoke.label, "'");
  }
  if (invoke.graph_detail) {
    const InvocationGraphicsDetail& graph_detail = *invoke.graph_detail;
    vkDestroyDescriptorPool(ctxt.dev, graph_detail.desc_pool, nullptr);
    log::debug("destroyed graphics invocation '", invoke.label, "'");
  }
  if (invoke.pass_detail) {
    const InvocationRenderPassDetail& pass_detail = *invoke.pass_detail;
    vkDestroyFramebuffer(ctxt.dev, pass_detail.framebuf, nullptr);
    log::debug("destroyed render pass invocation '", invoke.label, "'");
  }
  if (invoke.composite_detail) {
    log::debug("destroyed composite invocation '", invoke.label, "'");
  }

  if (invoke.query_pool != VK_NULL_HANDLE) {
    vkDestroyQueryPool(ctxt.dev, invoke.query_pool, nullptr);
    log::debug("destroyed timing objects");
  }

  if (invoke.bake_detail) {
    const InvocationBakingDetail& bake_detail = *invoke.bake_detail;
    vkDestroyCommandPool(ctxt.dev, bake_detail.cmd_pool, nullptr);
    log::debug("destroyed baking artifacts");
  }

  invoke = {};
}

double get_invoke_time_us(const Invocation& invoke) {
  if (invoke.query_pool == VK_NULL_HANDLE) { return 0.0; }
  uint64_t t[2];
  VK_ASSERT << vkGetQueryPoolResults(invoke.ctxt->dev, invoke.query_pool,
    0, 2, sizeof(uint64_t) * 2, &t, sizeof(uint64_t),
    VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT); // Wait till ready.
  double ns_per_tick = invoke.ctxt->physdev_prop.limits.timestampPeriod;
  return (t[1] - t[0]) * ns_per_tick / 1000.0;
}

VkSemaphore _create_sema(const Context& ctxt) {
  VkSemaphoreCreateInfo sci {};
  sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkSemaphore sema;
  VK_ASSERT << vkCreateSemaphore(ctxt.dev, &sci, nullptr, &sema);
  return sema;
}

VkCommandPool _create_cmd_pool(const Context& ctxt, SubmitType submit_ty) {
  VkCommandPoolCreateInfo cpci {};
  cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cpci.queueFamilyIndex = ctxt.submit_details.at(submit_ty).qfam_idx;

  VkCommandPool cmd_pool;
  VK_ASSERT << vkCreateCommandPool(ctxt.dev, &cpci, nullptr, &cmd_pool);

  return cmd_pool;
}
VkCommandBuffer _alloc_cmdbuf(
  const Context& ctxt,
  VkCommandPool cmd_pool,
  VkCommandBufferLevel level
) {
  VkCommandBufferAllocateInfo cbai {};
  cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cbai.level = level;
  cbai.commandBufferCount = 1;
  cbai.commandPool = cmd_pool;

  VkCommandBuffer cmdbuf;
  VK_ASSERT << vkAllocateCommandBuffers(ctxt.dev, &cbai, &cmdbuf);

  return cmdbuf;
}



struct TransactionLike {
  const Context* ctxt;
  std::vector<TransactionSubmitDetail> submit_details;
  VkCommandBufferLevel level;
  // Some invocations cannot be followedby subsequent invocations, e.g.
  // presentation.
  bool is_frozen;

  inline TransactionLike(const Context& ctxt, VkCommandBufferLevel level) :
    ctxt(&ctxt), submit_details(), level(level), is_frozen(false) {}
};
void _begin_cmdbuf(const TransactionSubmitDetail& submit_detail) {
  VkCommandBufferInheritanceInfo cbii {};
  cbii.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

  VkCommandBufferBeginInfo cbbi {};
  cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  if (submit_detail.submit_ty == L_SUBMIT_TYPE_GRAPHICS) {
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
  }
  cbbi.pInheritanceInfo = &cbii;
  VK_ASSERT << vkBeginCommandBuffer(submit_detail.cmdbuf, &cbbi);
}
void _end_cmdbuf(const TransactionSubmitDetail& submit_detail) {
  VK_ASSERT << vkEndCommandBuffer(submit_detail.cmdbuf);
}

void _push_transact_submit_detail(
  const Context& ctxt,
  std::vector<TransactionSubmitDetail>& submit_details,
  SubmitType submit_ty,
  VkCommandBufferLevel level
) {
  auto cmd_pool = _create_cmd_pool(ctxt, submit_ty);
  auto cmdbuf = _alloc_cmdbuf(ctxt, cmd_pool, level);

  TransactionSubmitDetail submit_detail {};
  submit_detail.submit_ty = submit_ty;
  submit_detail.cmd_pool = cmd_pool;
  submit_detail.cmdbuf = cmdbuf;
  submit_detail.queue = ctxt.submit_details.at(submit_detail.submit_ty).queue;
  submit_detail.wait_sema = submit_details.empty() ?
    VK_NULL_HANDLE : submit_details.back().signal_sema;
  if (level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
    submit_detail.signal_sema = VK_NULL_HANDLE;
  } else {
    submit_detail.signal_sema = _create_sema(ctxt);
  }

  submit_details.emplace_back(std::move(submit_detail));
}
void _submit_cmdbuf(
  TransactionLike& transact,
  VkFence fence
) {
  const Context& ctxt = *transact.ctxt;
  TransactionSubmitDetail& submit_detail = transact.submit_details.back();

  VkPipelineStageFlags stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

  VkSubmitInfo submit_info {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &submit_detail.cmdbuf;
  submit_info.signalSemaphoreCount = 1;
  if (submit_detail.wait_sema != VK_NULL_HANDLE) {
    // Wait for the last submitted command buffer on the device side.
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &submit_detail.wait_sema;
    submit_info.pWaitDstStageMask = &stage_mask;
  }
  submit_info.pSignalSemaphores = &submit_detail.signal_sema;

  // Finish recording and submit the command buffer to the device.
  VK_ASSERT << vkQueueSubmit(submit_detail.queue, 1, &submit_info, fence);

  submit_detail.is_submitted = true;
}
// `is_fenced` means that the command buffer is waited by the host so no
// semaphore signaling should be scheduled.
void _seal_cmdbuf(TransactionLike& transact) {
  if (!transact.submit_details.empty()) {
    // Do nothing if the queue is unchanged. It means that the commands can
    // still be fed into the last command buffer.
    auto& last_submit = transact.submit_details.back();

    if (last_submit.is_submitted) { return; }

    // End the command buffer and, it it's a primary command buffer, submit the
    // recorded commands.
    _end_cmdbuf(last_submit);
    if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
      _submit_cmdbuf(transact, VK_NULL_HANDLE);
    }
  }
}
VkCommandBuffer _get_cmdbuf(
  TransactionLike& transact,
  SubmitType submit_ty
) {
  if (submit_ty == L_SUBMIT_TYPE_ANY) {
    if (transact.submit_details.empty()) {
      submit_ty = transact.ctxt->submit_details.begin()->first;
    } else {
      submit_ty = transact.submit_details.back().submit_ty;
    }
  }
  const auto& submit_detail = transact.ctxt->submit_details.at(submit_ty);
  auto queue = submit_detail.queue;
  auto qfam_idx = submit_detail.qfam_idx;

  if (!transact.submit_details.empty()) {
    // Do nothing if the queue is unchanged. It means that the commands can
    // still be fed into the last command buffer.
    auto& last_submit = transact.submit_details.back();
    if (submit_detail.queue == last_submit.queue) {
      return last_submit.cmdbuf;
    }
  }

  // Otherwise, end the command buffer and, it it's a primary command buffer,
  // submit the recorded commands.
  _seal_cmdbuf(transact);

  _push_transact_submit_detail(*transact.ctxt, transact.submit_details,
    submit_ty, transact.level);
  _begin_cmdbuf(transact.submit_details.back());
  return transact.submit_details.back().cmdbuf;
}

void _make_buf_barrier_params(
  BufferUsage usage,
  VkAccessFlags& access,
  VkPipelineStageFlags& stage
) {
  switch (usage) {
  case L_BUFFER_USAGE_NONE:
    return; // Keep their original values.
  case L_BUFFER_USAGE_TRANSFER_SRC_BIT:
    access = VK_ACCESS_TRANSFER_READ_BIT;
    stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    break;
  case L_BUFFER_USAGE_TRANSFER_DST_BIT:
    access = VK_ACCESS_TRANSFER_WRITE_BIT;
    stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    break;
  case L_BUFFER_USAGE_UNIFORM_BIT:
    access = VK_ACCESS_UNIFORM_READ_BIT;
    stage =
      VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    break;
  case L_BUFFER_USAGE_STORAGE_BIT:
    access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    stage =
      VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    break;
  case L_BUFFER_USAGE_VERTEX_BIT:
    access = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
    break;
  case L_BUFFER_USAGE_INDEX_BIT:
    access = VK_ACCESS_INDEX_READ_BIT;
    stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
    break;
  default:
    panic("destination usage cannot be a set of bits");
  }
}
void _make_img_barrier_params(
  ImageUsage usage,
  VkAccessFlags& access,
  VkPipelineStageFlags& stage,
  VkImageLayout& layout
) {
  switch (usage) {
  case L_IMAGE_USAGE_NONE:
    return; // Keep their original values.
  case L_IMAGE_USAGE_TRANSFER_SRC_BIT:
    access = VK_ACCESS_TRANSFER_READ_BIT;
    stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    break;
  case L_IMAGE_USAGE_TRANSFER_DST_BIT:
    access = VK_ACCESS_TRANSFER_WRITE_BIT;
    stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    break;
  case L_IMAGE_USAGE_SAMPLED_BIT:
    access = VK_ACCESS_SHADER_READ_BIT;
    stage =
      VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    break;
  case L_IMAGE_USAGE_STORAGE_BIT:
    access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    stage =
      VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    layout = VK_IMAGE_LAYOUT_GENERAL;
    break;
  case L_IMAGE_USAGE_ATTACHMENT_BIT:
    access =
      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | // Blending does this.
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    break;
  case L_IMAGE_USAGE_SUBPASS_DATA_BIT:
    access = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    break;
  case L_IMAGE_USAGE_PRESENT_BIT:
    access = 0;
    stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    break;
  default:
    panic("destination usage cannot be a set of bits");
  }
}
// TODO: (penguinliong) Check these pipeline stages.
void _make_depth_img_barrier_params(
  DepthImageUsage usage,
  VkAccessFlags& access,
  VkPipelineStageFlags& stage,
  VkImageLayout& layout
) {
  switch (usage) {
  case L_DEPTH_IMAGE_USAGE_NONE:
    return; // Keep their original values.
  case L_DEPTH_IMAGE_USAGE_SAMPLED_BIT:
    access = VK_ACCESS_SHADER_READ_BIT;
    stage =
      VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    break;
  case L_DEPTH_IMAGE_USAGE_ATTACHMENT_BIT:
    access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    stage = 
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
      VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    break;
  case L_DEPTH_IMAGE_USAGE_SUBPASS_DATA_BIT:
    access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    stage =
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
      VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    break;
  default:
    panic("destination usage cannot be a set of bits");
  }
}
const BufferView& _transit_rsc(
  TransactionLike& transact,
  const BufferView& buf_view,
  BufferUsage dst_usage
) {
  auto& dyn_detail = (BufferDynamicDetail&)buf_view.buf->dyn_detail;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  VkAccessFlags src_access = dyn_detail.access;
  VkPipelineStageFlags src_stage = dyn_detail.stage;

  VkAccessFlags dst_access = 0;
  VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  _make_buf_barrier_params(dst_usage, dst_access, dst_stage);

  if (src_access == dst_access && src_stage == dst_stage) {
    return buf_view;
  }

  VkBufferMemoryBarrier bmb {};
  bmb.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bmb.buffer = buf_view.buf->buf;
  bmb.srcAccessMask = src_access;
  bmb.dstAccessMask = dst_access;
  bmb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bmb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bmb.offset = buf_view.offset;
  bmb.size = buf_view.size;

  vkCmdPipelineBarrier(
    cmdbuf,
    src_stage,
    dst_stage,
    0,
    0, nullptr,
    1, &bmb,
    0, nullptr);

  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    log::debug("inserted buffer barrier");
  }

  dyn_detail.access = dst_access;
  dyn_detail.stage = dst_stage;

  return buf_view;
}
const ImageView& _transit_rsc(
  TransactionLike& transact,
  const ImageView& img_view,
  ImageUsage dst_usage
) {
  auto& dyn_detail = (ImageDynamicDetail&)img_view.img->dyn_detail;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  VkAccessFlags src_access = dyn_detail.access;
  VkPipelineStageFlags src_stage = dyn_detail.stage;
  VkImageLayout src_layout = dyn_detail.layout;

  VkAccessFlags dst_access = 0;
  VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkImageLayout dst_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  _make_img_barrier_params(dst_usage, dst_access, dst_stage, dst_layout);

  if (
    src_access == dst_access &&
    src_stage == dst_stage &&
    src_layout == dst_layout
  ) {
    return img_view;
  }

  VkImageMemoryBarrier imb {};
  imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imb.image = img_view.img->img;
  imb.srcAccessMask = src_access;
  imb.dstAccessMask = dst_access;
  imb.oldLayout = src_layout;
  imb.newLayout = dst_layout;
  imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imb.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  // TODO: (penguinliong) Multi-layer image. 
  imb.subresourceRange.baseArrayLayer = 0;
  imb.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
  imb.subresourceRange.baseMipLevel = 0;
  imb.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

  vkCmdPipelineBarrier(
    cmdbuf,
    src_stage,
    dst_stage,
    0,
    0, nullptr,
    0, nullptr,
    1, &imb);

  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    log::debug("inserted image barrier");
  }

  dyn_detail.access = dst_access;
  dyn_detail.stage = dst_stage;
  dyn_detail.layout = dst_layout;

  return img_view;
}
const DepthImageView& _transit_rsc(
  TransactionLike& transact,
  const DepthImageView& depth_img_view,
  DepthImageUsage dst_usage
) {
  auto& dyn_detail =
    (DepthImageDynamicDetail&)depth_img_view.depth_img->dyn_detail;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  VkAccessFlags src_access = dyn_detail.access;
  VkPipelineStageFlags src_stage = dyn_detail.stage;
  VkImageLayout src_layout = dyn_detail.layout;

  VkAccessFlags dst_access = 0;
  VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkImageLayout dst_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  _make_depth_img_barrier_params(dst_usage, dst_access, dst_stage, dst_layout);

  if (
    src_access == dst_access &&
    src_stage == dst_stage &&
    src_layout == dst_layout
  ) {
    return depth_img_view;
  }

  VkImageMemoryBarrier imb {};
  imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imb.image = depth_img_view.depth_img->img;
  imb.srcAccessMask = src_access;
  imb.dstAccessMask = dst_access;
  imb.oldLayout = src_layout;
  imb.newLayout = dst_layout;
  imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imb.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  imb.subresourceRange.baseArrayLayer = 0;
  imb.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
  imb.subresourceRange.baseMipLevel = 0;
  imb.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

  vkCmdPipelineBarrier(
    cmdbuf,
    src_stage,
    dst_stage,
    0,
    0, nullptr,
    0, nullptr,
    1, &imb);

  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    log::debug("inserted depth image barrier");
  }

  dyn_detail.access = dst_access;
  dyn_detail.stage = dst_stage;
  dyn_detail.layout = dst_layout;

  return depth_img_view;
}
void _transit_rscs(
  TransactionLike& transact,
  const InvocationTransitionDetail& transit_detail
) {
  // Transition all referenced resources.
  for (auto& pair : transit_detail.buf_transit) {
    _transit_rsc(transact, pair.first, pair.second);
  }
  for (auto& pair : transit_detail.img_transit) {
    _transit_rsc(transact, pair.first, pair.second);
  }
  for (auto& pair : transit_detail.depth_img_transit) {
    _transit_rsc(transact, pair.first, pair.second);
  }
}

VkFence _create_fence(const Context& ctxt) {
  VkFenceCreateInfo fci {};
  fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  VkFence fence;
  VK_ASSERT << vkCreateFence(ctxt.dev, &fci, nullptr, &fence);
  return fence;
}
// Return true if the invocation forces an termination.
std::vector<VkFence> _record_invoke_impl(
  TransactionLike& transact,
  const Invocation& invoke
) {
  L_ASSERT(!transact.is_frozen, "invocations cannot be recorded while the "
    "transaction is frozen");

  if (invoke.present_detail) {
    L_ASSERT(transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      "present invocation cannot be baked");

    const InvocationPresentDetail& present_detail = *invoke.present_detail;
    Swapchain& swapchain = (Swapchain&)*present_detail.swapchain;
    const Context& ctxt = *swapchain.ctxt;

    // Transition image layout for presentation.
    uint32_t& img_idx = *swapchain.dyn_detail->img_idx;
    const Image& img = swapchain.dyn_detail->imgs[img_idx];
    ImageView img_view = make_img_view(img, 0, 0, img.img_cfg.width,
      img.img_cfg.height, img.img_cfg.depth, L_IMAGE_SAMPLER_NEAREST);
    _transit_rsc(transact, img_view, L_IMAGE_USAGE_PRESENT_BIT);


    // Present the rendered image.
    VkFence present_fence = _create_fence(ctxt);
    VkFence acquire_fence = _create_fence(ctxt);

    VkResult present_res = VK_SUCCESS;
    VkPresentInfoKHR pi {};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.swapchainCount = 1;
    pi.pSwapchains = &swapchain.swapchain;
    pi.pImageIndices = &img_idx;
    pi.pResults = &present_res;
    if (transact.submit_details.size() != 0) {
      const auto& last_submit = transact.submit_details.back();
      L_ASSERT(!last_submit.is_submitted);

      _end_cmdbuf(last_submit);
      _submit_cmdbuf(transact, present_fence);

      pi.waitSemaphoreCount = 1;
      pi.pWaitSemaphores = &transact.submit_details.back().signal_sema;
    }

    VkQueue queue = ctxt.submit_details.at(L_SUBMIT_TYPE_PRESENT).queue;
    VkResult res = vkQueuePresentKHR(queue, &pi);
    if (res == VK_SUBOPTIMAL_KHR) {
      // Request for swapchain recreation and suppress the result.
      swapchain.dyn_detail = nullptr;
      res = VK_SUCCESS;
    }
    VK_ASSERT << res;

    img_idx = ~0u;
    VK_ASSERT << vkAcquireNextImageKHR(ctxt.dev, swapchain.swapchain, 0,
      VK_NULL_HANDLE, acquire_fence, &img_idx);

    transact.is_frozen = true;

    log::debug("applied presentation invocation (image #", img_idx, ")");
    return { present_fence, acquire_fence };
  }

  VkCommandBuffer cmdbuf = _get_cmdbuf(transact, invoke.submit_ty);

  // If the invocation has been baked, simply inline the baked secondary command
  // buffer.
  if (invoke.bake_detail) {
    vkCmdExecuteCommands(cmdbuf, 1, &invoke.bake_detail->cmdbuf);
    return {};
  }

  if (invoke.query_pool != VK_NULL_HANDLE) {
    vkCmdResetQueryPool(cmdbuf, invoke.query_pool, 0, 2);
    vkCmdWriteTimestamp(cmdbuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      invoke.query_pool, 0);

    log::debug("invocation '", invoke.label, "' will be timed");
  }

  _transit_rscs(transact, invoke.transit_detail);

  if (invoke.b2b_detail) {
    const InvocationCopyBufferToBufferDetail& b2b_detail =
      *invoke.b2b_detail;
    vkCmdCopyBuffer(cmdbuf, b2b_detail.src, b2b_detail.dst, 1, &b2b_detail.bc);
    log::debug("applied transfer invocation '", invoke.label, "'");

  } else if (invoke.b2i_detail) {
    const InvocationCopyBufferToImageDetail& b2i_detail = *invoke.b2i_detail;
    vkCmdCopyBufferToImage(cmdbuf, b2i_detail.src, b2i_detail.dst,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &b2i_detail.bic);
    log::debug("applied transfer invocation '", invoke.label, "'");

  } else if (invoke.i2b_detail) {
    const InvocationCopyImageToBufferDetail& i2b_detail = *invoke.i2b_detail;
    vkCmdCopyImageToBuffer(cmdbuf, i2b_detail.src,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, i2b_detail.dst, 1, &i2b_detail.bic);
    log::debug("applied transfer invocation '", invoke.label, "'");

  } else if (invoke.i2i_detail) {
    const InvocationCopyImageToImageDetail& i2i_detail = *invoke.i2i_detail;
    vkCmdCopyImage(cmdbuf, i2i_detail.src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      i2i_detail.dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &i2i_detail.ic);
    log::debug("applied transfer invocation '", invoke.label, "'");

  } else if (invoke.comp_detail) {
    const InvocationComputeDetail& comp_detail = *invoke.comp_detail;
    const Task& task = *comp_detail.task;
    const DispatchSize& workgrp_count = comp_detail.workgrp_count;

    vkCmdBindPipeline(cmdbuf, comp_detail.bind_pt, task.pipe);
    if (comp_detail.desc_set != VK_NULL_HANDLE) {
      vkCmdBindDescriptorSets(cmdbuf, comp_detail.bind_pt, task.pipe_layout,
        0, 1, &comp_detail.desc_set, 0, nullptr);
    }
    vkCmdDispatch(cmdbuf, workgrp_count.x, workgrp_count.y, workgrp_count.z);
    log::debug("applied compute invocation '", invoke.label, "'");

  } else if (invoke.graph_detail) {
    const InvocationGraphicsDetail& graph_detail = *invoke.graph_detail;
    const Task& task = *graph_detail.task;

    vkCmdBindPipeline(cmdbuf, graph_detail.bind_pt, task.pipe);
    if (graph_detail.desc_set != VK_NULL_HANDLE) {
      vkCmdBindDescriptorSets(cmdbuf, graph_detail.bind_pt, task.pipe_layout,
        0, 1, &graph_detail.desc_set, 0, nullptr);
    }
    // TODO: (penguinliong) Vertex, index buffer transition.
    vkCmdBindVertexBuffers(cmdbuf, 0, (uint32_t)graph_detail.vert_bufs.size(),
      graph_detail.vert_bufs.data(), graph_detail.vert_buf_offsets.data());
    if (invoke.graph_detail->nidx != 0) {
      VkIndexType idx_ty {};
      switch (invoke.graph_detail->idx_ty) {
      case L_INDEX_TYPE_UINT16: idx_ty = VK_INDEX_TYPE_UINT16; break;
      case L_INDEX_TYPE_UINT32: idx_ty = VK_INDEX_TYPE_UINT32; break;
      default: panic("unexpected index type");
      }
      vkCmdBindIndexBuffer(cmdbuf, graph_detail.idx_buf,
        graph_detail.idx_buf_offset, idx_ty);
      vkCmdDrawIndexed(cmdbuf, graph_detail.nidx, graph_detail.ninst,
        0, 0, 0);
    } else {
      vkCmdDraw(cmdbuf, graph_detail.nvert, graph_detail.ninst, 0, 0);
    }
    log::debug("applied graphics invocation '", invoke.label, "'");

  } else if (invoke.pass_detail) {
    const InvocationRenderPassDetail& pass_detail = *invoke.pass_detail;
    const RenderPass& pass = *pass_detail.pass;
    const std::vector<const Invocation*>& subinvokes = pass_detail.subinvokes;

    VkSubpassContents sc = subinvokes.size() > 0 && subinvokes[0]->bake_detail ?
      VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS :
      VK_SUBPASS_CONTENTS_INLINE;

    VkRenderPassBeginInfo rpbi {};
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass = pass.pass;
    rpbi.framebuffer = pass_detail.framebuf;
    rpbi.renderArea.extent = pass.viewport.extent;
    rpbi.clearValueCount = (uint32_t)pass.clear_values.size();
    rpbi.pClearValues = pass.clear_values.data();
    vkCmdBeginRenderPass(cmdbuf, &rpbi, sc);
    log::debug("render pass invocation '", invoke.label, "' began");

    for (size_t i = 0; i < pass_detail.subinvokes.size(); ++i) {
      //if (i > 0) {
      //  sc = subinvokes[i]->bake_detail ?
      //    VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS :
      //    VK_SUBPASS_CONTENTS_INLINE;
      //  vkCmdNextSubpass(cmdbuf, sc);
      //  log::debug("render pass invocation '", invoke.label, "' switched to a "
      //    "next subpass");
      //}

      const Invocation* subinvoke = pass_detail.subinvokes[i];
      L_ASSERT(subinvoke != nullptr, "null subinvocation is not allowed");
      std::vector<VkFence> fences = _record_invoke_impl(transact, *subinvoke);
      if (!fences.empty()) { return fences; }
    }
    vkCmdEndRenderPass(cmdbuf);
    log::debug("render pass invocation '", invoke.label, "' ended");

  } else if (invoke.composite_detail) {
    const InvocationCompositeDetail& composite_detail =
      *invoke.composite_detail;

    log::debug("composite invocation '", invoke.label, "' began");

    for (size_t i = 0; i < composite_detail.subinvokes.size(); ++i) {
      const Invocation* subinvoke = composite_detail.subinvokes[i];
      L_ASSERT(subinvoke != nullptr, "null subinvocation is not allowed");
      std::vector<VkFence> fences = _record_invoke_impl(transact, *subinvoke);
      if (!fences.empty()) { return fences; }
    }

    log::debug("composite invocation '", invoke.label, "' ended");

  } else {
    unreachable();
  }

  if (invoke.query_pool != VK_NULL_HANDLE) {
    VkCommandBuffer cmdbuf2 = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);
    if (cmdbuf != cmdbuf2) {
      log::warn("begin and end timestamps are recorded in different command "
        "buffers, timing accuracy might be compromised");
    }
    vkCmdWriteTimestamp(cmdbuf2, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      invoke.query_pool, 1);
  }

  log::debug("scheduled invocation '", invoke.label, "' for execution");

  return {};
}
std::vector<VkFence> _record_invoke(
  TransactionLike& transact,
  const Invocation& invoke
) {
  std::vector<VkFence> fences = _record_invoke_impl(transact, invoke);
  if (fences.empty()) {
    _end_cmdbuf(transact.submit_details.back());
    if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
      VkFence fence = _create_fence(*transact.ctxt);
      _submit_cmdbuf(transact, fence);
      fences.emplace_back(fence);
    }
  }
  return fences;
}

bool _can_bake_invoke(const Invocation& invoke) {
  // Render pass is never baked, enforced by Vulkan specification.
  if (invoke.pass_detail != nullptr) { return false; }

  if (invoke.composite_detail != nullptr) {
    uint32_t submit_ty = ~uint32_t(0);
    for (const Invocation* subinvoke : invoke.composite_detail->subinvokes) {
      // If a subinvocation cannot be baked, this invocation too cannot.
      if (!_can_bake_invoke(*subinvoke)) { return false; }
      submit_ty &= subinvoke->submit_ty;
    }
    // Multiple subinvocations but their submit types mismatch.
    if (submit_ty == 0) { return 0; }
  }

  return true;
}
void bake_invoke(Invocation& invoke) {
  if (!_can_bake_invoke(invoke)) { return; }

  TransactionLike transact(*invoke.ctxt, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
  std::vector<VkFence> fences = _record_invoke(transact, invoke);
  L_ASSERT(fences.empty());

  L_ASSERT(transact.submit_details.size() == 1);
  const TransactionSubmitDetail& submit_detail = transact.submit_details[0];
  L_ASSERT(submit_detail.submit_ty == invoke.submit_ty);
  L_ASSERT(submit_detail.signal_sema == VK_NULL_HANDLE);

  InvocationBakingDetail bake_detail {};
  bake_detail.cmd_pool = submit_detail.cmd_pool;
  bake_detail.cmdbuf = submit_detail.cmdbuf;

  invoke.bake_detail =
    std::make_unique<InvocationBakingDetail>(std::move(bake_detail));

  log::debug("baked invocation '", invoke.label, "'");
}



Transaction submit_invoke(const Invocation& invoke) {
  const Context& ctxt = *invoke.ctxt;

  TransactionLike transact(ctxt, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  util::Timer timer {};
  timer.tic();
  std::vector<VkFence> fences = _record_invoke(transact, invoke);
  timer.toc();

  Transaction out {};
  out.ctxt = &ctxt;
  out.submit_details = std::move(transact.submit_details);
  out.fences = fences;

  log::debug("created and submitted transaction for execution, command "
    "recording took ", timer.us(), "us");
  return out;
}

} // namespace vk
} // namespace liong
