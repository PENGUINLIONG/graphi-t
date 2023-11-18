#include "sys.hpp"
#include "gft/log.hpp"
#include "gft/vk/vk-task.hpp"

namespace liong {
namespace vk {

TaskRef VulkanTask::create(const ContextRef &ctxt,
                           const ComputeTaskConfig &cfg) {
  L_ASSERT(cfg.workgrp_size.x * cfg.workgrp_size.y * cfg.workgrp_size.z != 0,
           "workgroup size cannot be zero");

  const VulkanContextRef &ctxt_ = VulkanContext::from_hal(ctxt);

  std::vector<VkDescriptorPoolSize> desc_pool_sizes;
  sys::DescriptorSetLayoutRef desc_set_layout =
      ctxt_->get_desc_set_layout(cfg.rsc_tys);
  sys::PipelineLayoutRef pipe_layout = sys::create_pipe_layout(*ctxt_->dev,
    desc_set_layout->desc_set_layout);
  VkShaderModule shader_mod = sys::create_shader_mod(*ctxt_->dev,
    (const uint32_t*)cfg.code, cfg.code_size);

  // Specialize to set local group size.
  VkSpecializationMapEntry spec_map_entries[] = {
    VkSpecializationMapEntry { 0, 0 * sizeof(int32_t), sizeof(int32_t) },
    VkSpecializationMapEntry { 1, 1 * sizeof(int32_t), sizeof(int32_t) },
    VkSpecializationMapEntry { 2, 2 * sizeof(int32_t), sizeof(int32_t) },
  };
  VkSpecializationInfo spec_info {};
  spec_info.pData = &cfg.workgrp_size;
  spec_info.dataSize = sizeof(cfg.workgrp_size);
  spec_info.mapEntryCount = 3;
  spec_info.pMapEntries = spec_map_entries;

  VkPipelineShaderStageCreateInfo pssci {};
  pssci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pssci.pName = cfg.entry_name.c_str();
  pssci.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  pssci.module = shader_mod;
  pssci.pSpecializationInfo = &spec_info;

  sys::PipelineRef pipe = sys::create_comp_pipe(*ctxt_->dev, pipe_layout->pipe_layout, pssci);

  sys::destroy_shader_mod(*ctxt_->dev, shader_mod);

  TaskResourceDetail rsc_detail {};
  rsc_detail.pipe_layout = std::move(pipe_layout);
  rsc_detail.rsc_tys = cfg.rsc_tys;

  TaskInfo info{};
  info.label = cfg.label;
  info.submit_ty = L_SUBMIT_TYPE_COMPUTE;

  VulkanTaskRef out = std::make_shared<VulkanTask>(ctxt_, std::move(info));
  out->ctxt = ctxt_;
  out->pass = nullptr;
  out->pipe = std::move(pipe);
  out->workgrp_size = cfg.workgrp_size;
  out->rsc_detail = std::move(rsc_detail);

  L_DEBUG("created compute task '", cfg.label, "'");

  return out;
}

TaskRef VulkanTask::create(const RenderPassRef &pass,
                           const GraphicsTaskConfig &cfg) {
  const VulkanRenderPassRef &pass_ = VulkanRenderPass::from_hal(pass);
  const VulkanContextRef& ctxt = pass_->ctxt;

  std::vector<VkDescriptorPoolSize> desc_pool_sizes;
  sys::DescriptorSetLayoutRef desc_set_layout =
      ctxt->get_desc_set_layout(cfg.rsc_tys);

  sys::PipelineLayoutRef pipe_layout = sys::create_pipe_layout(*ctxt->dev,
    desc_set_layout->desc_set_layout);
  VkShaderModule vert_shader_mod = sys::create_shader_mod(*ctxt->dev,
    (const uint32_t*)cfg.vert_code, cfg.vert_code_size);
  VkShaderModule frag_shader_mod = sys::create_shader_mod(*ctxt->dev,
    (const uint32_t*)cfg.frag_code, cfg.frag_code_size);

  VkPipelineInputAssemblyStateCreateInfo piasci {};
  piasci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  switch (cfg.topo) {
  case L_TOPOLOGY_POINT:
    piasci.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    break;
  case L_TOPOLOGY_LINE:
    piasci.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    break;
  case L_TOPOLOGY_TRIANGLE:
  case L_TOPOLOGY_TRIANGLE_WIREFRAME:
    piasci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    break;
  default:
    panic("unexpected topology (", cfg.topo, ")");
  }
  piasci.primitiveRestartEnable = VK_FALSE;

  VkPipelineRasterizationStateCreateInfo prsci {};
  prsci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  prsci.cullMode = VK_CULL_MODE_NONE;
  prsci.frontFace = VK_FRONT_FACE_CLOCKWISE;
  switch (cfg.topo) {
  case L_TOPOLOGY_TRIANGLE_WIREFRAME:
    prsci.polygonMode = VK_POLYGON_MODE_LINE;
    break;
  default:
    prsci.polygonMode = VK_POLYGON_MODE_FILL;
    break;
  }
  prsci.lineWidth = 1.0f;

  std::array<VkPipelineShaderStageCreateInfo, 2> psscis {};
  {
    VkPipelineShaderStageCreateInfo& pssci = psscis[0];
    pssci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pssci.pName = cfg.vert_entry_name.c_str();
    pssci.stage = VK_SHADER_STAGE_VERTEX_BIT;
    pssci.module = vert_shader_mod;
  }
  {
    VkPipelineShaderStageCreateInfo& pssci = psscis[1];
    pssci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pssci.pName = cfg.frag_entry_name.c_str();
    pssci.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pssci.module = frag_shader_mod;
  }

  sys::PipelineRef pipe = sys::create_graph_pipe(
      *ctxt->dev, pipe_layout->pipe_layout, *pass_->pass, pass_->info.width,
      pass->info.height, piasci, prsci, psscis);

  sys::destroy_shader_mod(*ctxt->dev, vert_shader_mod);
  sys::destroy_shader_mod(*ctxt->dev, frag_shader_mod);

  TaskResourceDetail rsc_detail {};
  rsc_detail.pipe_layout = std::move(pipe_layout);
  rsc_detail.rsc_tys = cfg.rsc_tys;

  TaskInfo info{};
  info.label = cfg.label;
  info.submit_ty = L_SUBMIT_TYPE_GRAPHICS;

  VulkanTaskRef out = std::make_shared<VulkanTask>(pass_, std::move(info));
  out->ctxt = ctxt;
  out->pass = pass_;
  out->pipe = std::move(pipe);
  out->workgrp_size = {};
  out->rsc_detail = std::move(rsc_detail);

  L_DEBUG("created graphics task '", cfg.label, "'");

  return out;
}
VulkanTask::~VulkanTask() {
  if (pipe) {
    L_DEBUG("destroyed task '", info.label, "'");
  }
}

} // namespace vk
} // namespace liong
