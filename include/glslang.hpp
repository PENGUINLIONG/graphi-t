// GLSL shader compilation.
// @PENGUINLIONG
#pragma once

namespace liong {

namespace glslang {

void initialize();

enum ShaderStage {
  L_SHADER_STAGE_COMPUTE,
  L_SHADER_STAGE_VERTEX,
  L_SHADER_STAGE_FRAGMENT,
};

struct ComputeSpirvArtifact {
  std::vector<uint32_t> comp_spv;
};
ComputeSpirvArtifact compile_comp(
  const std::string& comp_src,
  const std::string& comp_entry_point
);

struct GraphicsSpirvArtifact {
  std::vector<uint32_t> vert_spv;
  std::vector<uint32_t> frag_spv;
};
GraphicsSpirvArtifact compile_graph(
  const std::string& vert_src,
  const std::string& vert_entry_point,
  const std::string& frag_src,
  const std::string& frag_entry_point
);

} // namespace glslang

} // namespace liong
