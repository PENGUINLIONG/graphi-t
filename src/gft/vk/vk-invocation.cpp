#include "gft/vk/vk-invocation.hpp"
#include "gft/vk/vk-buffer.hpp"
#include "gft/vk/vk-image.hpp"
#include "gft/vk/vk-depth-image.hpp"
#include "gft/vk/vk-task.hpp"
#include "gft/vk/vk-transaction.hpp"
#include "gft/log.hpp"

namespace liong {
namespace vk {

void _update_desc_set(
  const VulkanContext& ctxt,
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
    const VulkanBuffer& buf = *VulkanBuffer::from_hal(buf_view.buf);

    VkDescriptorBufferInfo dbi{};
    dbi.buffer = *buf.buf;
    dbi.offset = buf_view.offset;
    dbi.range = buf_view.size;
    dbis.emplace_back(std::move(dbi));

    L_DEBUG(
      "bound pool resource #", wdss.size(), " to buffer '", buf.info.label, "'"
    );

    return &dbis.back();
  };
  auto push_dii = [&](const ResourceView& rsc_view, VkImageLayout layout) {
    VkDescriptorImageInfo dii{};
    if (rsc_view.rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE) {
      const ImageView& img_view = rsc_view.img_view;
      const VulkanImage& img = *VulkanImage::from_hal(img_view.img);

      dii.sampler = ctxt.img_samplers.at(img_view.sampler)->sampler;
      dii.imageView = *img.img_view;
      dii.imageLayout = layout;
      diis.emplace_back(std::move(dii));

      L_DEBUG(
        "bound pool resource #", wdss.size(), " to image '", img.info.label, "'"
      );
    } else if (rsc_view.rsc_view_ty == L_RESOURCE_VIEW_TYPE_DEPTH_IMAGE) {
      const DepthImageView& depth_img_view = rsc_view.depth_img_view;
      const VulkanDepthImage& depth_img =
        *VulkanDepthImage::from_hal(depth_img_view.depth_img);

      dii.sampler = ctxt.depth_img_samplers.at(depth_img_view.sampler)->sampler;
      dii.imageView = *depth_img.img_view;
      dii.imageLayout = layout;
      diis.emplace_back(std::move(dii));

      L_DEBUG(
        "bound pool resource #",
        wdss.size(),
        " to depth image '",
        depth_img.info.label,
        "'"
      );
    } else {
      panic();
    }

    return &diis.back();
  };

  for (uint32_t i = 0; i < rsc_views.size(); ++i) {
    const ResourceView& rsc_view = rsc_views[i];

    VkWriteDescriptorSet wds{};
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
        wds.pImageInfo =
          push_dii(rsc_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        break;
      case L_RESOURCE_TYPE_STORAGE_IMAGE:
        wds.pImageInfo = push_dii(rsc_view, VK_IMAGE_LAYOUT_GENERAL);
        wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        break;
      default:
        panic("unexpected resource type");
    }
    wdss.emplace_back(std::move(wds));
  }

  vkUpdateDescriptorSets(
    ctxt.dev->dev, (uint32_t)wdss.size(), wdss.data(), 0, nullptr
  );
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
          transit_detail.reg(
            rsc_view.depth_img_view, L_DEPTH_IMAGE_USAGE_SAMPLED_BIT
          );
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
      default:
        unreachable();
    }
  }
}
void _merge_subinvoke_transits(
  const std::vector<InvocationRef>& subinvokes,
  InvocationTransitionDetail& transit_detail
) {
  for (const auto& subinvoke : subinvokes) {
    const VulkanInvocationRef& subinvoke_ =
      VulkanInvocation::from_hal(subinvoke);
    L_ASSERT(subinvoke != nullptr);
    for (const auto& pair : subinvoke_->transit_detail.buf_transit) {
      transit_detail.buf_transit.emplace_back(pair);
    }
    for (const auto& pair : subinvoke_->transit_detail.img_transit) {
      transit_detail.img_transit.emplace_back(pair);
    }
    for (const auto& pair : subinvoke_->transit_detail.depth_img_transit) {
      transit_detail.depth_img_transit.emplace_back(pair);
    }
  }
}
void _merge_subinvoke_transits(
  const VulkanInvocationRef& subinvoke,
  InvocationTransitionDetail& transit_detail
) {
  std::vector<InvocationRef> subinvokes{subinvoke};
  _merge_subinvoke_transits(subinvokes, transit_detail);
}
SubmitType _infer_submit_ty(const std::vector<InvocationRef>& subinvokes) {
  for (size_t i = 0; i < subinvokes.size(); ++i) {
    SubmitType submit_ty = subinvokes[i]->info.submit_ty;
    if (submit_ty != L_SUBMIT_TYPE_ANY) {
      return submit_ty;
    }
  }
  return L_SUBMIT_TYPE_ANY;
}
VkBufferCopy _make_bc(const BufferView& src, const BufferView& dst) {
  VkBufferCopy bc{};
  bc.srcOffset = src.offset;
  bc.dstOffset = dst.offset;
  bc.size = dst.size;
  return bc;
}
VkImageCopy _make_ic(const ImageView& src, const ImageView& dst) {
  VkImageCopy ic{};
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
  VkBufferImageCopy bic{};
  bic.bufferOffset = buf.offset;
  bic.bufferRowLength = 0;
  bic.bufferImageHeight = (uint32_t)img.img->info.height;
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
  VulkanInvocation& out
) {
  InvocationCopyBufferToBufferDetail b2b_detail{};
  b2b_detail.bc = _make_bc(src, dst);
  b2b_detail.src = VulkanBuffer::from_hal(src.buf)->buf;
  b2b_detail.dst = VulkanBuffer::from_hal(dst.buf)->buf;

  out.b2b_detail =
    std::make_unique<InvocationCopyBufferToBufferDetail>(std::move(b2b_detail));

  out.transit_detail.reg(src, L_BUFFER_USAGE_TRANSFER_SRC_BIT);
  out.transit_detail.reg(dst, L_BUFFER_USAGE_TRANSFER_DST_BIT);
}
void _fill_transfer_b2i_invoke(
  const BufferView& src,
  const ImageView& dst,
  VulkanInvocation& out
) {
  InvocationCopyBufferToImageDetail b2i_detail{};
  b2i_detail.bic = _make_bic(src, dst);
  b2i_detail.src = VulkanBuffer::from_hal(src.buf)->buf;
  b2i_detail.dst = VulkanImage::from_hal(dst.img)->img;

  out.b2i_detail =
    std::make_unique<InvocationCopyBufferToImageDetail>(std::move(b2i_detail));

  out.transit_detail.reg(src, L_BUFFER_USAGE_TRANSFER_SRC_BIT);
  out.transit_detail.reg(dst, L_IMAGE_USAGE_TRANSFER_DST_BIT);
}
void _fill_transfer_i2b_invoke(
  const ImageView& src,
  const BufferView& dst,
  VulkanInvocation& out
) {
  InvocationCopyImageToBufferDetail i2b_detail{};
  i2b_detail.bic = _make_bic(dst, src);
  i2b_detail.src = VulkanImage::from_hal(src.img)->img;
  i2b_detail.dst = VulkanBuffer::from_hal(dst.buf)->buf;

  out.i2b_detail =
    std::make_unique<InvocationCopyImageToBufferDetail>(std::move(i2b_detail));

  out.transit_detail.reg(src, L_IMAGE_USAGE_TRANSFER_SRC_BIT);
  out.transit_detail.reg(dst, L_BUFFER_USAGE_TRANSFER_DST_BIT);
}
void _fill_transfer_i2i_invoke(
  const ImageView& src,
  const ImageView& dst,
  VulkanInvocation& out
) {
  InvocationCopyImageToImageDetail i2i_detail{};
  i2i_detail.ic = _make_ic(src, dst);
  i2i_detail.src = VulkanImage::from_hal(src.img)->img;
  i2i_detail.dst = VulkanImage::from_hal(dst.img)->img;

  out.i2i_detail =
    std::make_unique<InvocationCopyImageToImageDetail>(std::move(i2i_detail));

  out.transit_detail.reg(src, L_IMAGE_USAGE_TRANSFER_SRC_BIT);
  out.transit_detail.reg(dst, L_IMAGE_USAGE_TRANSFER_DST_BIT);
}
InvocationRef VulkanInvocation::create(
  const ContextRef& ctxt,
  const TransferInvocationConfig& cfg
) {
  const VulkanContextRef& ctxt_ = VulkanContext::from_hal(ctxt);

  const ResourceView& src_rsc_view = cfg.src_rsc_view;
  const ResourceView& dst_rsc_view = cfg.dst_rsc_view;
  ResourceViewType src_rsc_view_ty = src_rsc_view.rsc_view_ty;
  ResourceViewType dst_rsc_view_ty = dst_rsc_view.rsc_view_ty;

  InvocationInfo info{};
  info.label = cfg.label;
  info.submit_ty = L_SUBMIT_TYPE_TRANSFER;

  VulkanInvocationRef out =
    std::make_shared<VulkanInvocation>(ctxt_, std::move(info));
  out->ctxt = ctxt_;
  out->query_pool =
    cfg.is_timed ? ctxt_->acquire_query_pool() : QueryPoolPoolItem{};

  if (src_rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER && dst_rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER) {
    _fill_transfer_b2b_invoke(
      src_rsc_view.buf_view, dst_rsc_view.buf_view, *out
    );
  } else if (src_rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER && dst_rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE) {
    _fill_transfer_b2i_invoke(
      src_rsc_view.buf_view, dst_rsc_view.img_view, *out
    );
  } else if (src_rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE && dst_rsc_view_ty == L_RESOURCE_VIEW_TYPE_BUFFER) {
    _fill_transfer_i2b_invoke(
      src_rsc_view.img_view, dst_rsc_view.buf_view, *out
    );
  } else if (src_rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE && dst_rsc_view_ty == L_RESOURCE_VIEW_TYPE_IMAGE) {
    _fill_transfer_i2i_invoke(
      src_rsc_view.img_view, dst_rsc_view.img_view, *out
    );
  } else {
    panic("depth image cannot be transferred");
  }

  L_DEBUG("created transfer invocation");
  return out;
}
InvocationRef VulkanInvocation::create(
  const TaskRef& task,
  const ComputeInvocationConfig& cfg
) {
  const VulkanTaskRef& task_ = VulkanTask::from_hal(task);
  L_ASSERT(task_->rsc_detail.rsc_tys.size() == cfg.rsc_views.size());
  L_ASSERT(task_->info.submit_ty == L_SUBMIT_TYPE_COMPUTE);
  const VulkanContextRef& ctxt = task_->ctxt;

  InvocationInfo info{};
  info.label = cfg.label;
  info.submit_ty = L_SUBMIT_TYPE_COMPUTE;

  VulkanInvocationRef out =
    std::make_shared<VulkanInvocation>(ctxt, std::move(info));
  out->ctxt = ctxt;
  out->query_pool =
    cfg.is_timed ? ctxt->acquire_query_pool() : QueryPoolPoolItem{};

  InvocationTransitionDetail transit_detail{};
  _collect_task_invoke_transit(
    cfg.rsc_views, task_->rsc_detail.rsc_tys, transit_detail
  );
  out->transit_detail = std::move(transit_detail);

  InvocationComputeDetail comp_detail{};
  comp_detail.task = task_;
  comp_detail.bind_pt = VK_PIPELINE_BIND_POINT_COMPUTE;
  if (task_->rsc_detail.rsc_tys.size() > 0) {
    comp_detail.desc_set = ctxt->acquire_desc_set(task_->rsc_detail.rsc_tys);
    _update_desc_set(
      *ctxt,
      comp_detail.desc_set.value()->desc_set,
      task_->rsc_detail.rsc_tys,
      cfg.rsc_views
    );
  }
  comp_detail.workgrp_count = cfg.workgrp_count;

  out->comp_detail =
    std::make_unique<InvocationComputeDetail>(std::move(comp_detail));

  L_DEBUG("created compute invocation");
  return out;
}
InvocationRef VulkanInvocation::create(
  const TaskRef& task,
  const GraphicsInvocationConfig& cfg
) {
  const VulkanTaskRef& task_ = VulkanTask::from_hal(task);
  L_ASSERT(task_->rsc_detail.rsc_tys.size() == cfg.rsc_views.size());
  L_ASSERT(task_->info.submit_ty == L_SUBMIT_TYPE_GRAPHICS);
  const VulkanContextRef& ctxt = task_->ctxt;

  InvocationInfo info{};
  info.label = cfg.label;
  info.submit_ty = L_SUBMIT_TYPE_GRAPHICS;

  VulkanInvocationRef out =
    std::make_shared<VulkanInvocation>(ctxt, std::move(info));
  out->ctxt = ctxt;
  out->query_pool =
    cfg.is_timed ? ctxt->acquire_query_pool() : QueryPoolPoolItem{};

  InvocationTransitionDetail transit_detail{};
  _collect_task_invoke_transit(
    cfg.rsc_views, task_->rsc_detail.rsc_tys, transit_detail
  );
  for (size_t i = 0; i < cfg.vert_bufs.size(); ++i) {
    transit_detail.reg(cfg.vert_bufs[i], L_BUFFER_USAGE_VERTEX_BIT);
  }
  if (cfg.nidx > 0) {
    transit_detail.reg(cfg.idx_buf, L_BUFFER_USAGE_INDEX_BIT);
  }
  out->transit_detail = std::move(transit_detail);

  std::vector<sys::BufferRef> vert_bufs;
  std::vector<VkDeviceSize> vert_buf_offsets;
  vert_bufs.reserve(cfg.vert_bufs.size());
  vert_buf_offsets.reserve(cfg.vert_bufs.size());
  for (size_t i = 0; i < cfg.vert_bufs.size(); ++i) {
    const BufferView& vert_buf = cfg.vert_bufs[i];
    vert_bufs.emplace_back(VulkanBuffer::from_hal(vert_buf.buf)->buf);
    vert_buf_offsets.emplace_back(vert_buf.offset);
  }

  InvocationGraphicsDetail graph_detail{};
  graph_detail.task = task_;
  graph_detail.bind_pt = VK_PIPELINE_BIND_POINT_GRAPHICS;
  if (task_->rsc_detail.rsc_tys.size() > 0) {
    graph_detail.desc_set = ctxt->acquire_desc_set(task_->rsc_detail.rsc_tys);
    _update_desc_set(
      *ctxt,
      graph_detail.desc_set.value()->desc_set,
      task_->rsc_detail.rsc_tys,
      cfg.rsc_views
    );
  }
  graph_detail.vert_bufs = std::move(vert_bufs);
  graph_detail.vert_buf_offsets = std::move(vert_buf_offsets);
  if (cfg.idx_buf.buf != VK_NULL_HANDLE) {
    graph_detail.idx_buf = VulkanBuffer::from_hal(cfg.idx_buf.buf)->buf;
    graph_detail.idx_buf_offset = cfg.idx_buf.offset;
  }
  graph_detail.ninst = cfg.ninst;
  graph_detail.nvert = cfg.nvert;
  graph_detail.idx_ty = cfg.idx_ty;
  graph_detail.nidx = cfg.nidx;

  out->graph_detail =
    std::make_unique<InvocationGraphicsDetail>(std::move(graph_detail));

  L_DEBUG("created graphics invocation");
  return out;
}
InvocationRef VulkanInvocation::create(
  const RenderPassRef& pass,
  const RenderPassInvocationConfig& cfg
) {
  const VulkanRenderPassRef& pass_ = VulkanRenderPass::from_hal(pass);
  const VulkanContextRef& ctxt = pass_->ctxt;

  InvocationInfo info{};
  info.label = cfg.label;
  info.submit_ty = L_SUBMIT_TYPE_GRAPHICS;

  VulkanInvocationRef out =
    std::make_shared<VulkanInvocation>(ctxt, std::move(info));
  out->ctxt = ctxt;
  out->query_pool =
    cfg.is_timed ? ctxt->acquire_query_pool() : QueryPoolPoolItem{};

  InvocationTransitionDetail transit_detail{};
  for (size_t i = 0; i < cfg.attms.size(); ++i) {
    const ResourceView& attm = cfg.attms[i];

    switch (attm.rsc_view_ty) {
      case L_RESOURCE_VIEW_TYPE_IMAGE:
        transit_detail.reg(attm.img_view, L_IMAGE_USAGE_ATTACHMENT_BIT);
        break;
      case L_RESOURCE_VIEW_TYPE_DEPTH_IMAGE:
        transit_detail.reg(
          attm.depth_img_view, L_DEPTH_IMAGE_USAGE_ATTACHMENT_BIT
        );
        break;
      default:
        panic("render pass attachment must be image or depth image");
    }
  }
  _merge_subinvoke_transits(cfg.invokes, transit_detail);
  out->transit_detail = std::move(transit_detail);

  InvocationRenderPassDetail pass_detail{};
  pass_detail.pass = pass_;
  pass_detail.framebuf = pass_->acquire_framebuf(cfg.attms);
  // TODO: (penguinliong) Command buffer baking.
  pass_detail.is_baked = false;

  for (size_t i = 0; i < cfg.invokes.size(); ++i) {
    const InvocationRef& invoke = cfg.invokes[i];
    const VulkanInvocationRef& invoke_ = VulkanInvocation::from_hal(invoke);
    L_ASSERT(
      invoke_->graph_detail != nullptr,
      "render pass invocation constituent must be graphics task "
      "invocation"
    );
    pass_detail.subinvokes.emplace_back(invoke_);
  }

  out->pass_detail =
    std::make_unique<InvocationRenderPassDetail>(std::move(pass_detail));

  L_DEBUG("created render pass invocation");
  return out;
}
InvocationRef VulkanInvocation::create(
  const SwapchainRef& swapchain,
  const PresentInvocationConfig& cfg
) {
  const VulkanSwapchainRef& swapchain_ = VulkanSwapchain::from_hal(swapchain);

  L_ASSERT(
    swapchain_->dyn_detail != nullptr,
    "swapchain need to be recreated with `acquire_swapchain_img`"
  );

  const VulkanContextRef& ctxt = VulkanContext::from_hal(swapchain_->ctxt);
  SwapchainDynamicDetail& dyn_detail = *swapchain_->dyn_detail;

  L_ASSERT(
    dyn_detail.img_idx != nullptr,
    "swapchain has not acquired an image to present for the current "
    "frame"
  );

  InvocationInfo info{};
  info.label = util::format(swapchain_->info.label);
  info.submit_ty = L_SUBMIT_TYPE_PRESENT;

  VulkanInvocationRef out =
    std::make_shared<VulkanInvocation>(ctxt, std::move(info));
  out->ctxt = ctxt;
  out->query_pool = QueryPoolPoolItem{};

  InvocationPresentDetail present_detail{};
  present_detail.swapchain = swapchain_;

  out->present_detail =
    std::make_unique<InvocationPresentDetail>(std::move(present_detail));

  L_DEBUG("created present invocation");
  return out;
}
InvocationRef VulkanInvocation::create(
  const ContextRef& ctxt,
  const CompositeInvocationConfig& cfg
) {
  const VulkanContextRef& ctxt_ = VulkanContext::from_hal(ctxt);

  InvocationInfo info{};
  info.label = cfg.label;
  info.submit_ty = _infer_submit_ty(cfg.invokes);

  VulkanInvocationRef out =
    std::make_shared<VulkanInvocation>(ctxt_, std::move(info));
  out->ctxt = ctxt_;
  out->query_pool =
    cfg.is_timed ? ctxt_->acquire_query_pool() : QueryPoolPoolItem{};

  InvocationTransitionDetail transit_detail{};
  _merge_subinvoke_transits(cfg.invokes, transit_detail);
  out->transit_detail = std::move(transit_detail);

  InvocationCompositeDetail composite_detail{};
  for (size_t i = 0; i < cfg.invokes.size(); ++i) {
    const InvocationRef& invoke = cfg.invokes[i];
    const VulkanInvocationRef& invoke_ = VulkanInvocation::from_hal(invoke);
    composite_detail.subinvokes.emplace_back(invoke_);
  }

  out->composite_detail =
    std::make_unique<InvocationCompositeDetail>(std::move(composite_detail));

  L_DEBUG("created composition invocation");
  return out;
}
VulkanInvocation::VulkanInvocation(
  const VulkanContextRef& ctxt,
  InvocationInfo&& info
) :
  Invocation(std::move(info)), ctxt(ctxt) {}
VulkanInvocation::~VulkanInvocation() {
  if (b2b_detail || b2i_detail || i2b_detail || i2i_detail) {
    L_DEBUG("destroyed transfer invocation '", info.label, "'");
  }
  if (comp_detail) {
    L_DEBUG("destroyed compute invocation '", info.label, "'");
  }
  if (graph_detail) {
    L_DEBUG("destroyed graphics invocation '", info.label, "'");
  }
  if (pass_detail) {
    L_DEBUG("destroyed render pass invocation '", info.label, "'");
  }
  if (composite_detail) {
    L_DEBUG("destroyed composite invocation '", info.label, "'");
  }

  if (bake_detail) {
    L_DEBUG("destroyed baking artifacts");
  }
}

void _begin_cmdbuf(const TransactionSubmitDetail& submit_detail) {
  VkCommandBufferInheritanceInfo cbii{};
  cbii.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

  VkCommandBufferBeginInfo cbbi{};
  cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  if (submit_detail.submit_ty == L_SUBMIT_TYPE_GRAPHICS) {
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
  }
  cbbi.pInheritanceInfo = &cbii;
  VK_ASSERT << vkBeginCommandBuffer(submit_detail.cmdbuf->cmdbuf, &cbbi);
}
void _end_cmdbuf(const TransactionSubmitDetail& submit_detail) {
  VK_ASSERT << vkEndCommandBuffer(submit_detail.cmdbuf->cmdbuf);
}

sys::CommandBufferRef _alloc_cmdbuf(
  const VulkanContextRef& ctxt,
  VkCommandPool cmd_pool,
  VkCommandBufferLevel level
) {
  VkCommandBufferAllocateInfo cbai{};
  cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cbai.level = level;
  cbai.commandBufferCount = 1;
  cbai.commandPool = cmd_pool;

  return sys::CommandBuffer::create(*ctxt->dev, &cbai);
}

void _push_transact_submit_detail(
  const VulkanContextRef& ctxt,
  std::vector<TransactionSubmitDetail>& submit_details,
  SubmitType submit_ty,
  VkCommandBufferLevel level
) {
  auto cmd_pool = ctxt->acquire_cmd_pool(submit_ty);
  auto cmdbuf = _alloc_cmdbuf(ctxt, *cmd_pool.value(), level);

  TransactionSubmitDetail submit_detail{};
  submit_detail.submit_ty = submit_ty;
  submit_detail.cmd_pool = cmd_pool;
  submit_detail.cmdbuf = cmdbuf;
  submit_detail.queue = ctxt->submit_details.at(submit_detail.submit_ty).queue;
  submit_detail.wait_sema =
    submit_details.empty() ? VK_NULL_HANDLE : submit_details.back().signal_sema;
  if (level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
    submit_detail.signal_sema = VK_NULL_HANDLE;
  } else {
    submit_detail.signal_sema = sys::Semaphore::create(*ctxt->dev);
  }

  submit_details.emplace_back(std::move(submit_detail));
}
void _submit_cmdbuf(TransactionLike& transact, sys::FenceRef fence) {
  const Context& ctxt = *transact.ctxt;
  TransactionSubmitDetail& submit_detail = transact.submit_details.back();

  VkPipelineStageFlags stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &submit_detail.cmdbuf->cmdbuf;
  submit_info.signalSemaphoreCount = 1;
  if (submit_detail.wait_sema != VK_NULL_HANDLE) {
    // Wait for the last submitted command buffer on the device side.
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &submit_detail.wait_sema->sema;
    submit_info.pWaitDstStageMask = &stage_mask;
  }
  submit_info.pSignalSemaphores = &submit_detail.signal_sema->sema;

  // Finish recording and submit the command buffer to the device.
  VK_ASSERT << vkQueueSubmit(
    submit_detail.queue, 1, &submit_info, fence->fence
  );

  submit_detail.is_submitted = true;
}
// `is_fenced` means that the command buffer is waited by the host so no
// semaphore signaling should be scheduled.
void _seal_cmdbuf(TransactionLike& transact) {
  if (!transact.submit_details.empty()) {
    // Do nothing if the queue is unchanged. It means that the commands can
    // still be fed into the last command buffer.
    auto& last_submit = transact.submit_details.back();

    if (last_submit.is_submitted) {
      return;
    }

    // End the command buffer and, it it's a primary command buffer, submit the
    // recorded commands.
    _end_cmdbuf(last_submit);
    if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
      _submit_cmdbuf(transact, VK_NULL_HANDLE);
    }
  }
}
VkCommandBuffer _get_cmdbuf(TransactionLike& transact, SubmitType submit_ty) {
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
      return last_submit.cmdbuf->cmdbuf;
    }
  }

  // Otherwise, end the command buffer and, it it's a primary command buffer,
  // submit the recorded commands.
  _seal_cmdbuf(transact);

  _push_transact_submit_detail(
    transact.ctxt, transact.submit_details, submit_ty, transact.level
  );
  _begin_cmdbuf(transact.submit_details.back());
  return transact.submit_details.back().cmdbuf->cmdbuf;
}

void _make_buf_barrier_params(
  BufferUsage usage,
  VkAccessFlags& access,
  VkPipelineStageFlags& stage
) {
  switch (usage) {
    case L_BUFFER_USAGE_NONE:
      return;  // Keep their original values.
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
      stage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
              VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      break;
    case L_BUFFER_USAGE_STORAGE_BIT:
      access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      stage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
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
      return;  // Keep their original values.
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
      stage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
              VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      break;
    case L_IMAGE_USAGE_STORAGE_BIT:
      access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      stage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
              VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      layout = VK_IMAGE_LAYOUT_GENERAL;
      break;
    case L_IMAGE_USAGE_ATTACHMENT_BIT:
      access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |  // Blending does this.
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
      return;  // Keep their original values.
    case L_DEPTH_IMAGE_USAGE_SAMPLED_BIT:
      access = VK_ACCESS_SHADER_READ_BIT;
      stage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
              VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      break;
    case L_DEPTH_IMAGE_USAGE_ATTACHMENT_BIT:
      access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
              VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      break;
    case L_DEPTH_IMAGE_USAGE_SUBPASS_DATA_BIT:
      access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
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
  BufferDynamicDetail& dyn_detail =
    VulkanBuffer::from_hal(buf_view.buf)->dyn_detail;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  VkAccessFlags src_access = dyn_detail.access;
  VkPipelineStageFlags src_stage = dyn_detail.stage;

  VkAccessFlags dst_access = 0;
  VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  _make_buf_barrier_params(dst_usage, dst_access, dst_stage);

  if (src_access == dst_access && src_stage == dst_stage) {
    return buf_view;
  }

  VkBufferMemoryBarrier bmb{};
  bmb.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bmb.buffer = *VulkanBuffer::from_hal(buf_view.buf)->buf;
  bmb.srcAccessMask = src_access;
  bmb.dstAccessMask = dst_access;
  bmb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bmb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bmb.offset = buf_view.offset;
  bmb.size = buf_view.size;

  vkCmdPipelineBarrier(
    cmdbuf, src_stage, dst_stage, 0, 0, nullptr, 1, &bmb, 0, nullptr
  );

  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    L_DEBUG("inserted buffer barrier");
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
  ImageDynamicDetail& dyn_detail =
    VulkanImage::from_hal(img_view.img)->dyn_detail;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  VkAccessFlags src_access = dyn_detail.access;
  VkPipelineStageFlags src_stage = dyn_detail.stage;
  VkImageLayout src_layout = dyn_detail.layout;

  VkAccessFlags dst_access = 0;
  VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkImageLayout dst_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  _make_img_barrier_params(dst_usage, dst_access, dst_stage, dst_layout);

  if (src_access == dst_access && src_stage == dst_stage && src_layout == dst_layout) {
    return img_view;
  }

  VkImageMemoryBarrier imb{};
  imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imb.image = *VulkanImage::from_hal(img_view.img)->img;
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
    cmdbuf, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &imb
  );

  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    L_DEBUG("inserted image barrier");
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
  DepthImageDynamicDetail& dyn_detail =
    VulkanDepthImage::from_hal(depth_img_view.depth_img)->dyn_detail;
  auto cmdbuf = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);

  VkAccessFlags src_access = dyn_detail.access;
  VkPipelineStageFlags src_stage = dyn_detail.stage;
  VkImageLayout src_layout = dyn_detail.layout;

  VkAccessFlags dst_access = 0;
  VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkImageLayout dst_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  _make_depth_img_barrier_params(dst_usage, dst_access, dst_stage, dst_layout);

  if (src_access == dst_access && src_stage == dst_stage && src_layout == dst_layout) {
    return depth_img_view;
  }

  VkImageMemoryBarrier imb{};
  imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imb.image = *VulkanDepthImage::from_hal(depth_img_view.depth_img)->img;
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
    cmdbuf, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &imb
  );

  if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    L_DEBUG("inserted depth image barrier");
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

// Return true if the invocation forces an termination.
std::vector<sys::FenceRef> _record_invoke_impl(
  TransactionLike& transact,
  const VulkanInvocation& invoke
) {
  L_ASSERT(
    !transact.is_frozen,
    "invocations cannot be recorded while the "
    "transaction is frozen"
  );

  if (invoke.present_detail) {
    L_ASSERT(
      transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      "present invocation cannot be baked"
    );

    const InvocationPresentDetail& present_detail = *invoke.present_detail;
    const VulkanSwapchainRef& swapchain =
      VulkanSwapchain::from_hal(present_detail.swapchain);
    const VulkanContextRef& ctxt = swapchain->ctxt;

    // Transition image layout for presentation.
    uint32_t& img_idx = *swapchain->dyn_detail->img_idx;
    const VulkanImageRef& img = swapchain->dyn_detail->imgs[img_idx];
    ImageView img_view = img->view(L_IMAGE_SAMPLER_NEAREST);
    _transit_rsc(transact, img_view, L_IMAGE_USAGE_PRESENT_BIT);

    // Present the rendered image.
    sys::FenceRef present_fence = sys::Fence::create(*ctxt->dev);
    sys::FenceRef acquire_fence = sys::Fence::create(*ctxt->dev);

    VkResult present_res = VK_SUCCESS;
    VkPresentInfoKHR pi{};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.swapchainCount = 1;
    pi.pSwapchains = &swapchain->swapchain->swapchain;
    pi.pImageIndices = &img_idx;
    pi.pResults = &present_res;
    if (transact.submit_details.size() != 0) {
      const auto& last_submit = transact.submit_details.back();
      L_ASSERT(!last_submit.is_submitted);

      _end_cmdbuf(last_submit);
      _submit_cmdbuf(transact, present_fence);

      pi.waitSemaphoreCount = 1;
      pi.pWaitSemaphores = &transact.submit_details.back().signal_sema->sema;
    }

    VkQueue queue = ctxt->submit_details.at(L_SUBMIT_TYPE_PRESENT).queue;
    VkResult res = vkQueuePresentKHR(queue, &pi);
    if (res == VK_SUBOPTIMAL_KHR) {
      // Request for swapchain recreation and suppress the result.
      swapchain->dyn_detail = nullptr;
      res = VK_SUCCESS;
    }
    VK_ASSERT << res;

    img_idx = ~0u;
    res = VK_NOT_READY;
    do {
      res = vkAcquireNextImageKHR(
        *ctxt->dev,
        *swapchain->swapchain,
        SPIN_INTERVAL,
        VK_NULL_HANDLE,
        acquire_fence->fence,
        &img_idx
      );
      if (res == VK_NOT_READY) {
        L_DEBUG("failed to acquire image immediately");
      }
    } while (res == VK_TIMEOUT);
    VK_ASSERT << res;

    transact.is_frozen = true;

    L_DEBUG("applied presentation invocation (image #", img_idx, ")");
    return {present_fence, acquire_fence};
  }

  VkCommandBuffer cmdbuf = _get_cmdbuf(transact, invoke.info.submit_ty);

  // If the invocation has been baked, simply inline the baked secondary command
  // buffer.
  if (invoke.bake_detail) {
    vkCmdExecuteCommands(cmdbuf, 1, &invoke.bake_detail->cmdbuf->cmdbuf);
    return {};
  }

  if (invoke.query_pool.is_valid()) {
    vkCmdResetQueryPool(cmdbuf, *invoke.query_pool.value(), 0, 2);
    vkCmdWriteTimestamp(
      cmdbuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, *invoke.query_pool.value(), 0
    );

    L_DEBUG("invocation '", invoke.info.label, "' will be timed");
  }

  _transit_rscs(transact, invoke.transit_detail);

  if (invoke.b2b_detail) {
    const InvocationCopyBufferToBufferDetail& b2b_detail = *invoke.b2b_detail;
    vkCmdCopyBuffer(
      cmdbuf, b2b_detail.src->buf, b2b_detail.dst->buf, 1, &b2b_detail.bc
    );
    L_DEBUG("applied transfer invocation '", invoke.info.label, "'");
  } else if (invoke.b2i_detail) {
    const InvocationCopyBufferToImageDetail& b2i_detail = *invoke.b2i_detail;
    vkCmdCopyBufferToImage(
      cmdbuf,
      b2i_detail.src->buf,
      b2i_detail.dst->img,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &b2i_detail.bic
    );
    L_DEBUG("applied transfer invocation '", invoke.info.label, "'");
  } else if (invoke.i2b_detail) {
    const InvocationCopyImageToBufferDetail& i2b_detail = *invoke.i2b_detail;
    vkCmdCopyImageToBuffer(
      cmdbuf,
      i2b_detail.src->img,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      i2b_detail.dst->buf,
      1,
      &i2b_detail.bic
    );
    L_DEBUG("applied transfer invocation '", invoke.info.label, "'");
  } else if (invoke.i2i_detail) {
    const InvocationCopyImageToImageDetail& i2i_detail = *invoke.i2i_detail;
    vkCmdCopyImage(
      cmdbuf,
      i2i_detail.src->img,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      i2i_detail.dst->img,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &i2i_detail.ic
    );
    L_DEBUG("applied transfer invocation '", invoke.info.label, "'");
  } else if (invoke.comp_detail) {
    const InvocationComputeDetail& comp_detail = *invoke.comp_detail;
    const VulkanTaskRef& task = comp_detail.task;
    const DispatchSize& workgrp_count = comp_detail.workgrp_count;

    vkCmdBindPipeline(cmdbuf, comp_detail.bind_pt, task->pipe->pipe);
    if (comp_detail.desc_set.value()->desc_set != VK_NULL_HANDLE) {
      vkCmdBindDescriptorSets(
        cmdbuf,
        comp_detail.bind_pt,
        task->rsc_detail.pipe_layout->pipe_layout,
        0,
        1,
        &comp_detail.desc_set.value()->desc_set,
        0,
        nullptr
      );
    }
    vkCmdDispatch(cmdbuf, workgrp_count.x, workgrp_count.y, workgrp_count.z);
    L_DEBUG("applied compute invocation '", invoke.info.label, "'");
  } else if (invoke.graph_detail) {
    const InvocationGraphicsDetail& graph_detail = *invoke.graph_detail;
    const VulkanTaskRef& task = graph_detail.task;

    std::vector<VkBuffer> vert_bufs{};
    for (const auto& vert_buf : graph_detail.vert_bufs) {
      vert_bufs.emplace_back(vert_buf->buf);
    }

    vkCmdBindPipeline(cmdbuf, graph_detail.bind_pt, *task->pipe);
    if (graph_detail.desc_set.value()->desc_set != VK_NULL_HANDLE) {
      vkCmdBindDescriptorSets(
        cmdbuf,
        graph_detail.bind_pt,
        task->rsc_detail.pipe_layout->pipe_layout,
        0,
        1,
        &graph_detail.desc_set.value()->desc_set,
        0,
        nullptr
      );
    }
    // TODO: (penguinliong) Vertex, index buffer transition.
    vkCmdBindVertexBuffers(
      cmdbuf,
      0,
      (uint32_t)vert_bufs.size(),
      vert_bufs.data(),
      graph_detail.vert_buf_offsets.data()
    );
    if (invoke.graph_detail->nidx != 0) {
      VkIndexType idx_ty{};
      switch (invoke.graph_detail->idx_ty) {
        case L_INDEX_TYPE_UINT16:
          idx_ty = VK_INDEX_TYPE_UINT16;
          break;
        case L_INDEX_TYPE_UINT32:
          idx_ty = VK_INDEX_TYPE_UINT32;
          break;
        default:
          panic("unexpected index type");
      }
      vkCmdBindIndexBuffer(
        cmdbuf, graph_detail.idx_buf->buf, graph_detail.idx_buf_offset, idx_ty
      );
      vkCmdDrawIndexed(cmdbuf, graph_detail.nidx, graph_detail.ninst, 0, 0, 0);
    } else {
      vkCmdDraw(cmdbuf, graph_detail.nvert, graph_detail.ninst, 0, 0);
    }
    L_DEBUG("applied graphics invocation '", invoke.info.label, "'");
  } else if (invoke.pass_detail) {
    const InvocationRenderPassDetail& pass_detail = *invoke.pass_detail;
    const VulkanRenderPassRef& pass =
      VulkanRenderPass::from_hal(pass_detail.pass);
    const std::vector<VulkanInvocationRef>& subinvokes = pass_detail.subinvokes;

    VkSubpassContents sc = subinvokes.size() > 0 && subinvokes[0]->bake_detail
                             ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
                             : VK_SUBPASS_CONTENTS_INLINE;

    VkRenderPassBeginInfo rpbi{};
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass = *pass->pass;
    rpbi.framebuffer = pass_detail.framebuf.value()->framebuf;
    rpbi.renderArea.extent.width = pass->info.width;
    rpbi.renderArea.extent.height = pass->info.height;
    rpbi.clearValueCount = (uint32_t)pass->clear_values.size();
    rpbi.pClearValues = pass->clear_values.data();

    std::vector<VkImageView> img_views(pass_detail.attms.size());
    for (size_t i = 0; i < pass_detail.attms.size(); ++i) {
      img_views.at(i) = pass_detail.attms.at(i)->img_view;
    }

    vkCmdBeginRenderPass(cmdbuf, &rpbi, sc);
    L_DEBUG("render pass invocation '", invoke.info.label, "' began");

    for (size_t i = 0; i < pass_detail.subinvokes.size(); ++i) {
      // if (i > 0) {
      //   sc = subinvokes[i]->bake_detail ?
      //     VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS :
      //     VK_SUBPASS_CONTENTS_INLINE;
      //   vkCmdNextSubpass(cmdbuf, sc);
      //   L_DEBUG("render pass invocation '", label, "' switched to a "
      //     "next subpass");
      // }

      const VulkanInvocationRef& subinvoke = pass_detail.subinvokes[i];
      L_ASSERT(subinvoke != nullptr, "null subinvocation is not allowed");
      std::vector<sys::FenceRef> fences =
        _record_invoke_impl(transact, *subinvoke);
      if (!fences.empty()) {
        return fences;
      }
    }
    vkCmdEndRenderPass(cmdbuf);
    L_DEBUG("render pass invocation '", invoke.info.label, "' ended");
  } else if (invoke.composite_detail) {
    const InvocationCompositeDetail& composite_detail =
      *invoke.composite_detail;

    L_DEBUG("composite invocation '", invoke.info.label, "' began");

    for (size_t i = 0; i < composite_detail.subinvokes.size(); ++i) {
      const VulkanInvocationRef& subinvoke = composite_detail.subinvokes[i];
      L_ASSERT(subinvoke != nullptr, "null subinvocation is not allowed");
      std::vector<sys::FenceRef> fences =
        _record_invoke_impl(transact, *subinvoke);
      if (!fences.empty()) {
        return fences;
      }
    }

    L_DEBUG("composite invocation '", invoke.info.label, "' ended");
  } else {
    unreachable();
  }

  if (invoke.query_pool.is_valid()) {
    VkCommandBuffer cmdbuf2 = _get_cmdbuf(transact, L_SUBMIT_TYPE_ANY);
    if (cmdbuf != cmdbuf2) {
      L_WARN(
        "begin and end timestamps are recorded in different command "
        "buffers, timing accuracy might be compromised"
      );
    }
    vkCmdWriteTimestamp(
      cmdbuf2, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, *invoke.query_pool.value(), 1
    );
  }

  L_DEBUG("scheduled invocation '", invoke.info.label, "' for execution");

  return {};
}
void _record_invoke(TransactionLike& transact, const VulkanInvocation& invoke) {
  transact.fences = _record_invoke_impl(transact, invoke);
  if (transact.fences.empty()) {
    _end_cmdbuf(transact.submit_details.back());
    if (transact.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
      sys::FenceRef fence = sys::Fence::create(transact.ctxt->dev->dev);
      _submit_cmdbuf(transact, fence);
      transact.fences.emplace_back(std::move(fence));
    }
  }
}

bool _can_bake_invoke(const VulkanInvocation& invoke) {
  // Render pass is never baked, enforced by Vulkan specification.
  if (invoke.pass_detail != nullptr) {
    return false;
  }

  if (invoke.composite_detail != nullptr) {
    uint32_t submit_ty = ~uint32_t(0);
    for (const VulkanInvocationRef& subinvoke :
         invoke.composite_detail->subinvokes) {
      // If a subinvocation cannot be baked, this invocation too cannot.
      if (!_can_bake_invoke(*subinvoke)) {
        return false;
      }
      submit_ty &= subinvoke->info.submit_ty;
    }
    // Multiple subinvocations but their submit types mismatch.
    if (submit_ty == 0) {
      return 0;
    }
  }

  return true;
}

void VulkanInvocation::record(TransactionLike& transact) const {
  _record_invoke(transact, *this);
}

TransactionRef VulkanInvocation::create_transact(const TransactionConfig& cfg) {
  return VulkanTransaction::create(shared_from_this(), cfg);
}

double VulkanInvocation::get_time_us() {
  if (!query_pool.is_valid()) {
    return 0.0;
  }
  uint64_t t[2];
  VK_ASSERT << vkGetQueryPoolResults(
    ctxt->dev->dev,
    *query_pool.value(),
    0,
    2,
    sizeof(uint64_t) * 2,
    &t,
    sizeof(uint64_t),
    VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
  );  // Wait till
      // ready.
  double ns_per_tick = ctxt->physdev_prop().limits.timestampPeriod;
  return (t[1] - t[0]) * ns_per_tick / 1000.0;
}

void VulkanInvocation::bake() {
  if (!_can_bake_invoke(*this)) {
    return;
  }

  TransactionLike transact(ctxt, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
  record(transact);
  L_ASSERT(transact.fences.empty());

  L_ASSERT(transact.submit_details.size() == 1);
  const TransactionSubmitDetail& submit_detail = transact.submit_details[0];
  L_ASSERT(submit_detail.submit_ty == info.submit_ty);
  L_ASSERT(submit_detail.signal_sema == VK_NULL_HANDLE);

  bake_detail = std::make_unique<InvocationBakingDetail>();
  bake_detail->cmd_pool = submit_detail.cmd_pool;
  bake_detail->cmdbuf = submit_detail.cmdbuf;

  L_DEBUG("baked invocation '", info.label, "'");
}

}  // namespace vk
}  // namespace liong
