#include "gft/vk.hpp"
#include "gft/log.hpp"

namespace liong {
namespace vk {

VkDescriptorSetLayout _create_desc_set_layout(
  const Context& ctxt,
  const std::vector<ResourceType> rsc_tys,
  std::vector<VkDescriptorPoolSize>& desc_pool_sizes
) {
  std::vector<VkDescriptorSetLayoutBinding> dslbs;
  std::map<VkDescriptorType, uint32_t> desc_counter;
  for (auto i = 0; i < rsc_tys.size(); ++i) {
    const auto& rsc_ty = rsc_tys[i];

    VkDescriptorSetLayoutBinding dslb {};
    dslb.binding = i;
    dslb.descriptorCount = 1;
    dslb.stageFlags =
      VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT;
    switch (rsc_ty) {
    case L_RESOURCE_TYPE_UNIFORM_BUFFER:
      dslb.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      break;
    case L_RESOURCE_TYPE_STORAGE_BUFFER:
      dslb.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      break;
    case L_RESOURCE_TYPE_SAMPLED_IMAGE:
      dslb.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      break;
    case L_RESOURCE_TYPE_STORAGE_IMAGE:
      dslb.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      break;
    default:
      panic("unexpected resource type");
    }
    desc_counter[dslb.descriptorType] += 1;

    dslbs.emplace_back(std::move(dslb));

    // Collect `desc_pool_sizes` for checks on resource bindings.
    for (const auto& pair : desc_counter) {
      VkDescriptorPoolSize desc_pool_size;
      desc_pool_size.type = pair.first;
      desc_pool_size.descriptorCount = pair.second;
      desc_pool_sizes.emplace_back(std::move(desc_pool_size));
    }
  }

  VkDescriptorSetLayoutCreateInfo dslci {};
  dslci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  dslci.bindingCount = (uint32_t)dslbs.size();
  dslci.pBindings = dslbs.data();

  VkDescriptorSetLayout desc_set_layout;
  VK_ASSERT <<
    vkCreateDescriptorSetLayout(ctxt.dev, &dslci, nullptr, &desc_set_layout);

  return desc_set_layout;
}
VkPipelineLayout _create_pipe_layout(
  const Context& ctxt,
  VkDescriptorSetLayout desc_set_layout
) {
  VkPipelineLayoutCreateInfo plci {};
  plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  plci.setLayoutCount = 1;
  plci.pSetLayouts = &desc_set_layout;

  VkPipelineLayout pipe_layout;
  VK_ASSERT << vkCreatePipelineLayout(ctxt.dev, &plci, nullptr, &pipe_layout);

  return pipe_layout;
}
VkShaderModule _create_shader_mod(
  const Context& ctxt,
  const void* code,
  size_t code_size
) {
  VkShaderModuleCreateInfo smci {};
  smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  smci.pCode = (const uint32_t*)code;
  smci.codeSize = code_size;

  VkShaderModule shader_mod;
  VK_ASSERT << vkCreateShaderModule(ctxt.dev, &smci, nullptr, &shader_mod);

  return shader_mod;
}
Task create_comp_task(
  const Context& ctxt,
  const ComputeTaskConfig& cfg
) {
  assert(cfg.workgrp_size.x * cfg.workgrp_size.y * cfg.workgrp_size.z != 0,
    "workgroup size cannot be zero");
  std::vector<VkDescriptorPoolSize> desc_pool_sizes;
  VkDescriptorSetLayout desc_set_layout = _create_desc_set_layout(ctxt,
    cfg.rsc_tys, desc_pool_sizes);
  VkPipelineLayout pipe_layout = _create_pipe_layout(ctxt, desc_set_layout);
  VkShaderModule shader_mod = _create_shader_mod(ctxt, cfg.code, cfg.code_size);

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

  VkComputePipelineCreateInfo cpci {};
  cpci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  cpci.stage = std::move(pssci);
  cpci.layout = pipe_layout;

  VkPipeline pipe;
  VK_ASSERT << vkCreateComputePipelines(ctxt.dev, VK_NULL_HANDLE, 1, &cpci,
    nullptr, &pipe);

  log::debug("created compute task '", cfg.label, "'");
  return Task {
    cfg.label, L_SUBMIT_TYPE_COMPUTE, &ctxt, nullptr, desc_set_layout,
    pipe_layout, pipe, cfg.rsc_tys, { shader_mod }, std::move(desc_pool_sizes)
  };
}



Task create_graph_task(
  const RenderPass& pass,
  const GraphicsTaskConfig& cfg
) {
  const Context& ctxt = *pass.ctxt;

  std::vector<VkDescriptorPoolSize> desc_pool_sizes;
  VkDescriptorSetLayout desc_set_layout =
    _create_desc_set_layout(ctxt, cfg.rsc_tys, desc_pool_sizes);
  VkPipelineLayout pipe_layout = _create_pipe_layout(ctxt, desc_set_layout);
  VkShaderModule vert_shader_mod =
    _create_shader_mod(ctxt, cfg.vert_code, cfg.vert_code_size);
  VkShaderModule frag_shader_mod =
    _create_shader_mod(ctxt, cfg.frag_code, cfg.frag_code_size);

  VkPrimitiveTopology topo;
  VkPolygonMode poly_mode;
  switch (cfg.topo) {
  case L_TOPOLOGY_POINT:
    topo = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    poly_mode = VK_POLYGON_MODE_FILL;
    break;
  case L_TOPOLOGY_LINE:
    topo = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    poly_mode = VK_POLYGON_MODE_FILL;
    break;
  case L_TOPOLOGY_TRIANGLE:
    topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    poly_mode = VK_POLYGON_MODE_FILL;
    break;
  case L_TOPOLOGY_TRIANGLE_WIREFRAME:
    topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    poly_mode = VK_POLYGON_MODE_LINE;
    break;
  default:
    panic("unexpected topology (", cfg.topo, ")");
  }

  VkPipelineShaderStageCreateInfo psscis[2] {};
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

  VkVertexInputBindingDescription vibd {};
  std::vector<VkVertexInputAttributeDescription> viads;
  size_t base_offset = 0;
  for (auto i = 0; i < cfg.vert_inputs.size(); ++i) {
    auto& vert_input = cfg.vert_inputs[i];
    size_t fmt_size = fmt::get_fmt_size(vert_input.fmt);

    vibd.binding = 0;
    vibd.stride = 0; // Will be assigned later.
    switch (cfg.vert_inputs[i].rate) {
    case L_VERTEX_INPUT_RATE_VERTEX:
      vibd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      break;
    case L_VERTEX_INPUT_RATE_INSTANCE:
      vibd.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
      panic("instanced draw is currently unsupported");
      break;
    default:
      panic("unexpected vertex input rate");
    }

    VkVertexInputAttributeDescription viad {};
    viad.location = i;
    viad.binding = 0;
    viad.format = fmt2vk(vert_input.fmt, fmt::L_COLOR_SPACE_LINEAR);
    viad.offset = (uint32_t)base_offset;
    viads.emplace_back(std::move(viad));

    base_offset += fmt_size;
  }
  vibd.stride = (uint32_t)base_offset;

  VkPipelineVertexInputStateCreateInfo pvisci {};
  pvisci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  pvisci.vertexBindingDescriptionCount = 1;
  pvisci.pVertexBindingDescriptions = &vibd;
  pvisci.vertexAttributeDescriptionCount = (uint32_t)viads.size();
  pvisci.pVertexAttributeDescriptions = viads.data();

  VkPipelineInputAssemblyStateCreateInfo piasci {};
  piasci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  piasci.topology = topo;
  piasci.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport {};
  viewport.x = 0;
  viewport.y = 0;
  viewport.width = (float)pass.viewport.extent.width;
  viewport.height = (float)pass.viewport.extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  VkRect2D scissor {};
  scissor.offset = pass.viewport.offset;
  scissor.extent = pass.viewport.extent;
  VkPipelineViewportStateCreateInfo pvsci {};
  pvsci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  pvsci.viewportCount = 1;
  pvsci.pViewports = &viewport;
  pvsci.scissorCount = 1;
  pvsci.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo prsci {};
  prsci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  prsci.cullMode = VK_CULL_MODE_NONE;
  prsci.frontFace = VK_FRONT_FACE_CLOCKWISE;
  prsci.polygonMode = VK_POLYGON_MODE_FILL;
  prsci.lineWidth = 1.0f;

  VkPipelineMultisampleStateCreateInfo pmsci {};
  pmsci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  pmsci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo pdssci {};
  pdssci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  pdssci.depthTestEnable = VK_TRUE;
  pdssci.depthWriteEnable = VK_TRUE;
  pdssci.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  pdssci.minDepthBounds = 0.0f;
  pdssci.maxDepthBounds = 1.0f;

  // TODO: (penguinliong) Support multiple attachments.
  std::array<VkPipelineColorBlendAttachmentState, 1> pcbass {};
  {
    VkPipelineColorBlendAttachmentState& pcbas = pcbass[0];
    pcbas.blendEnable = VK_FALSE;
    pcbas.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT |
      VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT |
      VK_COLOR_COMPONENT_A_BIT;
  }
  VkPipelineColorBlendStateCreateInfo pcbsci {};
  pcbsci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  pcbsci.attachmentCount = (uint32_t)pcbass.size();
  pcbsci.pAttachments = pcbass.data();

  VkPipelineDynamicStateCreateInfo pdsci {};
  pdsci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  pdsci.dynamicStateCount = 0;
  pdsci.pDynamicStates = nullptr;

  VkGraphicsPipelineCreateInfo gpci {};
  gpci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  gpci.stageCount = 2;
  gpci.pStages = psscis;
  gpci.pVertexInputState = &pvisci;
  gpci.pInputAssemblyState = &piasci;
  gpci.pTessellationState = nullptr;
  gpci.pViewportState = &pvsci;
  gpci.pRasterizationState = &prsci;
  gpci.pMultisampleState = &pmsci;
  gpci.pDepthStencilState = &pdssci;
  gpci.pColorBlendState = &pcbsci;
  gpci.pDynamicState = &pdsci;
  gpci.layout = pipe_layout;
  gpci.renderPass = pass.pass;
  gpci.subpass = 0;

  VkPipeline pipe;
  VK_ASSERT << vkCreateGraphicsPipelines(ctxt.dev, VK_NULL_HANDLE, 1, &gpci,
    nullptr, &pipe);

  log::debug("created graphics task '", cfg.label, "'");
  return Task {
    cfg.label, L_SUBMIT_TYPE_GRAPHICS, &ctxt, &pass, desc_set_layout,
    pipe_layout, pipe, cfg.rsc_tys, { vert_shader_mod, frag_shader_mod },
    std::move(desc_pool_sizes)
  };
}
void destroy_task(Task& task) {
  if (task.pipe != VK_NULL_HANDLE) {
    vkDestroyPipeline(task.ctxt->dev, task.pipe, nullptr);
    for (const VkShaderModule& shader_mod : task.shader_mods) {
      vkDestroyShaderModule(task.ctxt->dev, shader_mod, nullptr);
    }
    task.shader_mods.clear();
    vkDestroyPipelineLayout(task.ctxt->dev, task.pipe_layout, nullptr);
    vkDestroyDescriptorSetLayout(task.ctxt->dev, task.desc_set_layout, nullptr);

    log::debug("destroyed task '", task.label, "'");
    task = {};
  }
}

} // namespace vk
} // namespace liong
