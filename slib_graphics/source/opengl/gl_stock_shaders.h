#pragma once
#include "material_system.h"
#include "renderer.h"

namespace lib_graphics {
class GlStockShaders {
 public:
  GlStockShaders();
  ~GlStockShaders() = default;

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
};
}  // namespace lib_graphics
