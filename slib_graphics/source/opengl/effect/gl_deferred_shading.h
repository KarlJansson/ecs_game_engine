#pragma once
#include "culling_system.h"
#include "engine_core.h"
#include "entity_manager.h"

namespace lib_graphics {
class GlDeferredShading {
 public:
  GlDeferredShading(lib_core::EngineCore *engine);
  ~GlDeferredShading() = default;

  void DrawGBuffers(const Camera cam,
                    const ct::dyn_array<CullingSystem::MeshPack> &mesh_packs);
  void DrawTranslucents(
      const Camera cam,
      const ct::dyn_array<CullingSystem::MeshPack> &mesh_packs);

 private:
  lib_core::EngineCore *engine_;

  ct::hash_map<unsigned, std::array<int, 9>> shader_locations_;
  int draw_calls_, max_inst_;
};
}  // namespace lib_graphics
