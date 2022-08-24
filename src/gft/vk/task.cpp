#include "gft/vk.hpp"
#include "gft/log.hpp"
#include "sys.hpp"

namespace liong {
namespace vk {

VkDescriptorSetLayout _create_desc_set_layout(
  const Context& ctxt,
  const std::vector<ResourceType>& rsc_tys
) {
  uint32_t nuniform_buf {};
  uint32_t nstorage_buf {};
  uint32_t nsampled_img {};
  uint32_t nstorage_img {};
  std::vector<VkDescriptorSetLayoutBinding> dslbs;
  for (auto i = 0; i < rsc_tys.size(); ++i) {
    const auto& rsc_ty = rsc_tys[i];

    VkDescriptorSetLayoutBinding dslb {};
    dslb.binding = i;
    dslb.descriptorCount = 1;
    dslb.stageFlags =
      VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT;
    switch (rsc_ty) {
    case L_RESOURCE_TYPE_UNIFORM_BUFFER:
      ++nuniform_buf;
      dslb.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      break;
    case L_RESOURCE_TYPE_STORAGE_BUFFER:
      ++nstorage_buf;
      dslb.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      break;
    case L_RESOURCE_TYPE_SAMPLED_IMAGE:
      ++nsampled_img;
      dslb.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      break;
    case L_RESOURCE_TYPE_STORAGE_IMAGE:
      ++nstorage_img;
      dslb.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      break;
    default:
      panic("unexpected resource type");
    }
    dslbs.emplace_back(std::move(dslb));
  }

  L_ASSERT(nuniform_buf < 256);
  L_ASSERT(nstorage_buf < 256);
  L_ASSERT(nsampled_img < 256);
  L_ASSERT(nstorage_img < 256);

  DescriptorCounter desc_counter {};
  std::array<VkDescriptorPoolSize, 4> desc_pool_sizes {};

  desc_counter.counters.nuniform_buf = (uint8_t)nuniform_buf;
  desc_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  desc_pool_sizes[0].descriptorCount = nuniform_buf;

  desc_counter.counters.nstorage_buf = (uint8_t)nstorage_buf;
  desc_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  desc_pool_sizes[1].descriptorCount = nstorage_buf;

  desc_counter.counters.nsampled_img = (uint8_t)nsampled_img;
  desc_pool_sizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  desc_pool_sizes[2].descriptorCount = nsampled_img;

  desc_counter.counters.nstorage_img = (uint8_t)nstorage_img;
  desc_pool_sizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  desc_pool_sizes[3].descriptorCount = nstorage_img;

  VkDescriptorSetLayout desc_set_layout;
  auto it = ctxt.desc_pool_detail.desc_set_layouts.find(desc_counter);
  if (it == ctxt.desc_pool_detail.desc_set_layouts.end()) {
    desc_set_layout = sys::create_desc_set_layout(ctxt.dev, dslbs);

    DescriptorCounter aligned_desc_counter {desc_counter};
    aligned_desc_counter.counters.nuniform_buf = util::div_up(nuniform_buf, DescriptorPoolClass::NRESOURCE_ALIGN);
    aligned_desc_counter.counters.nstorage_buf = util::div_up(nstorage_buf, DescriptorPoolClass::NRESOURCE_ALIGN);
    aligned_desc_counter.counters.nsampled_img = util::div_up(nsampled_img, DescriptorPoolClass::NRESOURCE_ALIGN);
    aligned_desc_counter.counters.nstorage_img = util::div_up(nstorage_img, DescriptorPoolClass::NRESOURCE_ALIGN);

    std::array<VkDescriptorPoolSize, 4> aligned_desc_pool_sizes {desc_pool_sizes};
    aligned_desc_pool_sizes[0].descriptorCount = aligned_desc_counter.counters.nuniform_buf;
    aligned_desc_pool_sizes[1].descriptorCount = aligned_desc_counter.counters.nstorage_buf;
    aligned_desc_pool_sizes[2].descriptorCount = aligned_desc_counter.counters.nsampled_img;
    aligned_desc_pool_sizes[3].descriptorCount = aligned_desc_counter.counters.nstorage_img;

    DescriptorPoolClass& desc_pool_class =
      const_cast<Context&>(ctxt).desc_pool_detail.desc_pool_classes[desc_set_layout];
    desc_pool_class.aligned_desc_counter = aligned_desc_counter;
    desc_pool_class.aligned_desc_pool_sizes = aligned_desc_pool_sizes;

    const_cast<Context&>(ctxt).desc_pool_detail.desc_set_layouts[desc_counter] = desc_set_layout;
  } else {
    desc_set_layout = it->second;
  }

  return desc_set_layout;
}
Task create_comp_task(
  const Context& ctxt,
  const ComputeTaskConfig& cfg
) {
  L_ASSERT(cfg.workgrp_size.x * cfg.workgrp_size.y * cfg.workgrp_size.z != 0,
    "workgroup size cannot be zero");
  VkDescriptorSetLayout desc_set_layout = _create_desc_set_layout(ctxt,
    cfg.rsc_tys);
  VkPipelineLayout pipe_layout = sys::create_pipe_layout(ctxt.dev,
    desc_set_layout);
  VkShaderModule shader_mod = sys::create_shader_mod(ctxt.dev,
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

  VkPipeline pipe = sys::create_comp_pipe(ctxt.dev, pipe_layout, pssci);

  sys::destroy_shader_mod(ctxt.dev, shader_mod);

  TaskResourceDetail rsc_detail {};
  rsc_detail.desc_set_layout = desc_set_layout;
  rsc_detail.pipe_layout = pipe_layout;
  rsc_detail.rsc_tys = cfg.rsc_tys;

  log::debug("created compute task '", cfg.label, "'");
  return Task {
    cfg.label, L_SUBMIT_TYPE_COMPUTE, const_cast<Context*>(&ctxt), nullptr, pipe,
    cfg.workgrp_size, std::move(rsc_detail)
  };
}



Task create_graph_task(
  const RenderPass& pass,
  const GraphicsTaskConfig& cfg
) {
  const Context& ctxt = *pass.ctxt;

  VkDescriptorSetLayout desc_set_layout =
    _create_desc_set_layout(ctxt, cfg.rsc_tys);
  VkPipelineLayout pipe_layout = sys::create_pipe_layout(ctxt.dev,
    desc_set_layout);
  VkShaderModule vert_shader_mod = sys::create_shader_mod(ctxt.dev,
    (const uint32_t*)cfg.vert_code, cfg.vert_code_size);
  VkShaderModule frag_shader_mod = sys::create_shader_mod(ctxt.dev,
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

  VkPipeline pipe = sys::create_graph_pipe(ctxt.dev, pipe_layout, pass.pass,
    pass.width, pass.height, piasci, prsci, psscis);

  sys::destroy_shader_mod(ctxt.dev, vert_shader_mod);
  sys::destroy_shader_mod(ctxt.dev, frag_shader_mod);

  TaskResourceDetail rsc_detail {};
  rsc_detail.desc_set_layout = desc_set_layout;
  rsc_detail.pipe_layout = pipe_layout;
  rsc_detail.rsc_tys = cfg.rsc_tys;

  log::debug("created graphics task '", cfg.label, "'");
  return Task {
    cfg.label, L_SUBMIT_TYPE_GRAPHICS, const_cast<Context*>(&ctxt), &pass, pipe, {},
    std::move(rsc_detail)
  };
}
void destroy_task(Task& task) {
  VkDevice dev = task.ctxt->dev;

  if (task.pipe != VK_NULL_HANDLE) {
    sys::destroy_pipe(dev, task.pipe);
    sys::destroy_pipe_layout(dev, task.rsc_detail.pipe_layout);

    log::debug("destroyed task '", task.label, "'");
    task = {};
  }
}

} // namespace vk
} // namespace liong
