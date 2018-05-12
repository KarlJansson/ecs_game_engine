#pragma once
#include "material_system.h"
#include "renderer.h"

namespace lib_graphics {
class GlStockShaders {
 public:
  GlStockShaders();
  ~GlStockShaders() = default;

  void CommonShaderSnippets();
  void ForwardShaders();
  void DeferredShaders();

  void CompileShaders(MaterialSystem *mat_mgr);
  size_t GetShaderId(MaterialSystem::ShaderType type);
  size_t DefToForId(size_t deferred_id);

 private:
  ct::hash_map<MaterialSystem::ShaderType,
               std::tuple<ct::string, ct::string, ct::string>>
      shader_source_;
  ct::hash_map<MaterialSystem::ShaderType, size_t> forward_shader_ids_,
      deferred_shader_ids_;

  ct::hash_map<size_t, size_t> def_to_for_map_;

  ct::string vertex_header_, matrix_input_, pbr_util_funcs_,
      pbr_util_funcs_shadows_end_, pbr_util_funcs_end_, pbr_lighting_dir_,
      pbr_lighting_attenuation_, point_shadows_, pbr_helper_funcs_,
      pbr_lighting_spot_;
};
}  // namespace lib_graphics
