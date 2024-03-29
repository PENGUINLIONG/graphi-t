// GLSL shader compilation.
// @PENGUINLIONG
#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace liong {

namespace glslang {

enum GlslangTarget {
  L_GLSLANG_TARGET_VULKAN_1_0,
  L_GLSLANG_TARGET_VULKAN_1_1,
  L_GLSLANG_TARGET_VULKAN_1_2,
};

void initialize(GlslangTarget target);
void initialize();

struct ComputeSpirvArtifact {
  std::vector<uint32_t> comp_spv;
  size_t ubo_size;
};
ComputeSpirvArtifact compile_comp(
  const std::string& comp_src,
  const std::string& comp_entry_point
);
ComputeSpirvArtifact compile_comp_hlsl(
  const std::string& comp_src,
  const std::string& comp_entry_point
);

struct GraphicsSpirvArtifact {
  std::vector<uint32_t> vert_spv;
  std::vector<uint32_t> frag_spv;
  size_t ubo_size;
};
GraphicsSpirvArtifact compile_graph(
  const std::string& vert_src,
  const std::string& vert_entry_point,
  const std::string& frag_src,
  const std::string& frag_entry_point
);
GraphicsSpirvArtifact compile_graph_hlsl(
  const std::string& vert_src,
  const std::string& vert_entry_point,
  const std::string& frag_src,
  const std::string& frag_entry_point
);

} // namespace glslang

} // namespace liong
