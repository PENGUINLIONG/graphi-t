#include "gft/vk.hpp"
#include "gft/log.hpp"

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
VkRenderPass _create_pass(
  const Context& ctxt,
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
      ad.format = fmt2vk(attm_cfg.color_fmt, attm_cfg.cspace);
      ad.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      ad.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      sar.color_attm_ref.emplace_back(std::move(ar));
      break;
    }
    case L_ATTACHMENT_TYPE_DEPTH:
    {
      assert(!sar.has_depth_attm, "subpass can only have one depth attachment");
      ar.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      ad.format = depth_fmt2vk(attm_cfg.depth_fmt);
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

  VkRenderPass pass;
  VK_ASSERT << vkCreateRenderPass(ctxt.dev, &rpci, nullptr, &pass);

  return pass;
}

RenderPass create_pass(const Context& ctxt, const RenderPassConfig& cfg) {
  VkRenderPass pass = _create_pass(ctxt, cfg.attm_cfgs);

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

  log::debug("created render pass '", cfg.label, "'");
  return RenderPass { &ctxt, viewport, pass, cfg, clear_values };
}
void destroy_pass(RenderPass& pass) {
  vkDestroyRenderPass(pass.ctxt->dev, pass.pass, nullptr);
  log::debug("destroyed render pass '", pass.pass_cfg.label, "'");
}

} // namespace vk
} // namespcae liong
