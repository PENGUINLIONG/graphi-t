#include "gft/vk.hpp"
#include "gft/log.hpp"
#include "sys.hpp"

namespace liong {
namespace vk {

sys::DescriptorSetLayoutRef _create_desc_set_layout(
  const Context& ctxt,
  const std::vector<ResourceType>& rsc_tys,
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

  return sys::create_desc_set_layout(ctxt.dev->dev, dslbs);
}

sys::DescriptorPoolRef _create_desc_pool(
  const Context& ctxt,
  const std::vector<VkDescriptorPoolSize>& desc_pool_sizes
) {
  VkDescriptorPoolCreateInfo dpci {};
  dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  dpci.poolSizeCount = static_cast<uint32_t>(desc_pool_sizes.size());
  dpci.pPoolSizes = desc_pool_sizes.data();
  dpci.maxSets = 1;

  return sys::DescriptorPool::create(ctxt.dev, &dpci);
}
sys::DescriptorSetRef _alloc_desc_set(
  const Context& ctxt,
  VkDescriptorPool desc_pool,
  VkDescriptorSetLayout desc_set_layout
) {
  VkDescriptorSetAllocateInfo dsai {};
  dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dsai.descriptorPool = desc_pool;
  dsai.descriptorSetCount = 1;
  dsai.pSetLayouts = &desc_set_layout;

  return sys::DescriptorSet::create(ctxt.dev, &dsai);
}

Task create_comp_task(
  const Context& ctxt,
  const ComputeTaskConfig& cfg
) {
  L_ASSERT(cfg.workgrp_size.x * cfg.workgrp_size.y * cfg.workgrp_size.z != 0,
    "workgroup size cannot be zero");
  std::vector<VkDescriptorPoolSize> desc_pool_sizes;
  sys::DescriptorSetLayoutRef desc_set_layout = _create_desc_set_layout(ctxt,
    cfg.rsc_tys, desc_pool_sizes);
  sys::PipelineLayoutRef pipe_layout = sys::create_pipe_layout(ctxt.dev->dev,
    desc_set_layout->desc_set_layout);
  VkShaderModule shader_mod = sys::create_shader_mod(ctxt.dev->dev,
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

  sys::PipelineRef pipe = sys::create_comp_pipe(ctxt.dev->dev, pipe_layout->pipe_layout, pssci);

  sys::destroy_shader_mod(ctxt.dev->dev, shader_mod);

  TaskResourceDetail rsc_detail {};
  rsc_detail.desc_set_layout = std::move(desc_set_layout);
  rsc_detail.pipe_layout = std::move(pipe_layout);
  rsc_detail.rsc_tys = cfg.rsc_tys;
  rsc_detail.desc_pool_sizes = std::move(desc_pool_sizes);

  log::debug("created compute task '", cfg.label, "'");
  return Task {
    cfg.label, L_SUBMIT_TYPE_COMPUTE, &ctxt, nullptr, std::move(pipe),
    cfg.workgrp_size, std::move(rsc_detail)
  };
}



Task create_graph_task(
  const RenderPass& pass,
  const GraphicsTaskConfig& cfg
) {
  const Context& ctxt = *pass.ctxt;

  std::vector<VkDescriptorPoolSize> desc_pool_sizes;
  sys::DescriptorSetLayoutRef desc_set_layout =
    _create_desc_set_layout(ctxt, cfg.rsc_tys, desc_pool_sizes);
  sys::PipelineLayoutRef pipe_layout = sys::create_pipe_layout(ctxt.dev->dev,
    desc_set_layout->desc_set_layout);
  VkShaderModule vert_shader_mod = sys::create_shader_mod(ctxt.dev->dev,
    (const uint32_t*)cfg.vert_code, cfg.vert_code_size);
  VkShaderModule frag_shader_mod = sys::create_shader_mod(ctxt.dev->dev,
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

  sys::PipelineRef pipe = sys::create_graph_pipe(ctxt.dev->dev,
    pipe_layout->pipe_layout, pass.pass, pass.width, pass.height, piasci, prsci,
    psscis);

  sys::destroy_shader_mod(ctxt.dev->dev, vert_shader_mod);
  sys::destroy_shader_mod(ctxt.dev->dev, frag_shader_mod);

  TaskResourceDetail rsc_detail {};
  rsc_detail.desc_set_layout = std::move(desc_set_layout);
  rsc_detail.pipe_layout = std::move(pipe_layout);
  rsc_detail.rsc_tys = cfg.rsc_tys;
  rsc_detail.desc_pool_sizes = std::move(desc_pool_sizes);

  log::debug("created graphics task '", cfg.label, "'");
  return Task {
    cfg.label, L_SUBMIT_TYPE_GRAPHICS, &ctxt, &pass, std::move(pipe), {},
    std::move(rsc_detail)
  };
}
void destroy_task(Task& task) {
  VkDevice dev = task.ctxt->dev->dev;

  if (task.pipe != VK_NULL_HANDLE) {
    log::debug("destroyed task '", task.label, "'");
    task = {};
  }
}

TaskDescriptorSetPoolItemRef::TaskDescriptorSetPoolItemRef(
  Task* task
) : task(task), desc_set(task->acquire_desc_set()) {}
TaskDescriptorSetPoolItemRef::~TaskDescriptorSetPoolItemRef() {
  task->release_desc_set(std::exchange(desc_set, {}));
}

sys::DescriptorSetRef Task::acquire_desc_set() {
  if (rsc_detail.desc_pool_items.size() > 0) {
    auto back = std::move(rsc_detail.desc_pool_items.back());
    rsc_detail.desc_pool_items.pop_back();
    return back;
  } else {
    if (rsc_detail.desc_pool_sizes.size() > 0) {
      return _create_desc_set(*ctxt, rsc_detail.desc_pool_sizes, rsc_detail.desc_set_layout->desc_set_layout);
    } else {
      return nullptr;
    }
  }
}
void Task::release_desc_set(sys::DescriptorSetRef&& item) {
  rsc_detail.desc_pool_items.emplace_back(std::move(item));
}

} // namespace vk
} // namespace liong
