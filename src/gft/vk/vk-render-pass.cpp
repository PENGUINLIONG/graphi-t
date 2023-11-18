#include "gft/log.hpp"
#include "gft/vk/vk-render-pass.hpp"
#include "gft/vk/vk-image.hpp"
#include "gft/vk/vk-depth-image.hpp"

namespace liong {
namespace vk {

VkAttachmentLoadOp _get_load_op(AttachmentAccess attm_access) {
  if (attm_access & L_ATTACHMENT_ACCESS_CLEAR) {
    return VK_ATTACHMENT_LOAD_OP_CLEAR;
  }
  if (attm_access & L_ATTACHMENT_ACCESS_LOAD) {
    return VK_ATTACHMENT_LOAD_OP_LOAD;
  }
  return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}
VkAttachmentStoreOp _get_store_op(AttachmentAccess attm_access) {
  if (attm_access & L_ATTACHMENT_ACCESS_STORE) {
    return VK_ATTACHMENT_STORE_OP_STORE;
  }
  return VK_ATTACHMENT_STORE_OP_DONT_CARE;
}
sys::RenderPassRef _create_pass(
  const VulkanContext& ctxt,
  const std::vector<AttachmentConfig>& attm_cfgs
) {
  struct SubpassAttachmentReference {
    std::vector<VkAttachmentReference> color_attm_ref;
    bool has_depth_attm;
    VkAttachmentReference depth_attm_ref;
  };

  SubpassAttachmentReference sar {};
  std::vector<VkAttachmentDescription> ads;
  for (uint32_t i = 0; i < attm_cfgs.size(); ++i) {
    const AttachmentConfig& attm_cfg = attm_cfgs.at(i);
    uint32_t iattm = i;

    VkAttachmentReference ar {};
    ar.attachment = i;
    VkAttachmentDescription ad {};
    ad.samples = VK_SAMPLE_COUNT_1_BIT;
    ad.loadOp = _get_load_op(attm_cfg.attm_access);
    ad.storeOp = _get_store_op(attm_cfg.attm_access);
    switch (attm_cfg.attm_ty) {
    case L_ATTACHMENT_TYPE_COLOR:
    {
      ar.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      ad.format = format2vk(attm_cfg.color_fmt, attm_cfg.cspace);
      ad.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      ad.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      sar.color_attm_ref.emplace_back(std::move(ar));
      break;
    }
    case L_ATTACHMENT_TYPE_DEPTH:
    {
      L_ASSERT(!sar.has_depth_attm,
        "subpass can only have one depth attachment");
      ar.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      ad.format = depth_format2vk(attm_cfg.depth_fmt);
      ad.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      ad.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      sar.has_depth_attm = true;
      sar.depth_attm_ref = std::move(ar);
      break;
    }
    default: unreachable();
    }

    ads.emplace_back(std::move(ad));
  }

  // TODO: (penguinliong) Support input attachments.
  std::vector<VkSubpassDescription> sds;
  {
    VkSubpassDescription sd {};
    sd.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sd.inputAttachmentCount = 0;
    sd.pInputAttachments = nullptr;
    sd.colorAttachmentCount = (uint32_t)sar.color_attm_ref.size();
    sd.pColorAttachments = sar.color_attm_ref.data();
    sd.pResolveAttachments = nullptr;
    sd.pDepthStencilAttachment =
      sar.has_depth_attm ? &sar.depth_attm_ref : nullptr;
    sd.preserveAttachmentCount = 0;
    sd.pPreserveAttachments = nullptr;
    sds.emplace_back(std::move(sd));
  }
  VkRenderPassCreateInfo rpci {};
  rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  rpci.attachmentCount = (uint32_t)ads.size();
  rpci.pAttachments = ads.data();
  rpci.subpassCount = (uint32_t)sds.size();
  rpci.pSubpasses = sds.data();
  // TODO: (penguinliong) Implement subpass dependency resolution in the future.
  rpci.dependencyCount = 0;
  rpci.pDependencies = nullptr;

  return sys::RenderPass::create(ctxt.dev->dev, &rpci);
}

sys::FramebufferRef _create_framebuf(
  const VulkanRenderPass& pass,
  const std::vector<ResourceView>& attms
) {
  L_ASSERT(pass.info.attm_count == attms.size());

  uint32_t width = pass.info.width;
  uint32_t height = pass.info.height;

  std::vector<VkImageView> img_views(attms.size());
  for (size_t i = 0; i < attms.size(); ++i) {
    const ResourceView& attm = attms.at(i);
    switch (attm.rsc_view_ty) {
    case L_RESOURCE_VIEW_TYPE_IMAGE:
      img_views.at(i) = *VulkanImage::from_hal(attm.img_view.img)->img_view;
      break;
    case L_RESOURCE_VIEW_TYPE_DEPTH_IMAGE:
      img_views.at(i) = *VulkanDepthImage::from_hal(attm.depth_img_view.depth_img)->img_view;
      break;
    default:
      L_PANIC("unexpected attachment resource view type");
    }
  }

  VkFramebufferCreateInfo fci {};
  fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fci.attachmentCount = img_views.size();
  fci.pAttachments = img_views.data();
  fci.renderPass = pass.pass->pass;
  fci.width = width;
  fci.height = height;
  fci.layers = 1;

  return sys::Framebuffer::create(pass.ctxt->dev->dev, &fci);
}

RenderPassRef VulkanRenderPass::create(const ContextRef &ctxt,
                                       const RenderPassConfig &cfg) {
  VulkanContextRef ctxt_ = VulkanContext::from_hal(ctxt);
  
  sys::RenderPassRef pass = _create_pass(*ctxt_, cfg.attm_cfgs);

  VkRect2D viewport {};
  viewport.extent.width = cfg.width;
  viewport.extent.height = cfg.height;

  std::vector<VkClearValue> clear_values {};
  clear_values.resize(cfg.attm_cfgs.size());
  for (size_t i = 0; i < cfg.attm_cfgs.size(); ++i) {
    auto& clear_value = clear_values[i];
    switch (cfg.attm_cfgs[i].attm_ty) {
    case L_ATTACHMENT_TYPE_COLOR:
      clear_value.color.float32[0] = 0.0f;
      clear_value.color.float32[0] = 0.0f;
      clear_value.color.float32[0] = 0.0f;
      clear_value.color.float32[0] = 0.0f;
      break;
    case L_ATTACHMENT_TYPE_DEPTH:
      clear_value.depthStencil.depth = 1.0f;
      clear_value.depthStencil.stencil = 0;
      break;
    default: panic();
    }
  }

  RenderPassInfo info{};
  info.label = cfg.label;
  info.width = cfg.width;
  info.height = cfg.height;
  info.attm_count = cfg.attm_cfgs.size();

  VulkanRenderPassRef out = std::make_shared<VulkanRenderPass>(ctxt_, std::move(info));
  out->ctxt = ctxt_;
  out->pass = std::move(pass);
  out->clear_values = clear_values;

  L_DEBUG("created render pass '", cfg.label, "'");

  return std::static_pointer_cast<RenderPass>(out);
}
VulkanRenderPass::~VulkanRenderPass() {
  if (pass) {
    L_DEBUG("destroyed render pass '", info.label, "'");
  }
}

FramebufferKey FramebufferKey::create(
  const VulkanRenderPass& pass,
  const std::vector<ResourceView>& rsc_views
) {
  std::stringstream ss;
  ss << *pass.pass;
  for (const auto& rsc_view : rsc_views) {
    switch (rsc_view.rsc_view_ty) {
    case L_RESOURCE_VIEW_TYPE_IMAGE:
    {
      VkImageView img_view = VulkanImage::from_hal(rsc_view.img_view.img)->img_view->img_view;
      ss << "," << img_view;
      break;
    }
    case L_RESOURCE_VIEW_TYPE_DEPTH_IMAGE:
    {
      VkImageView img_view = VulkanDepthImage::from_hal(rsc_view.depth_img_view.depth_img)->img_view->img_view;
      ss << "," << img_view;
      break;
    }
    default: L_PANIC("unsupported resource type as an attachment");
    }
  }
  return { ss.str() };
}

FramebufferPoolItem VulkanRenderPass::acquire_framebuf(
  const std::vector<ResourceView>& attms
) {
  FramebufferKey key = FramebufferKey::create(*this, attms);
  if (framebuf_pool.has_free_item(key)) {
    return framebuf_pool.acquire(std::move(key));
  } else {
    return framebuf_pool.create(std::move(key), _create_framebuf(*this, attms));
  }
}

} // namespace vk
} // namespcae liong
