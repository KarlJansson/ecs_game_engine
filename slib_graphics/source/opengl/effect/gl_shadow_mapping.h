#pragma once
#include "engine_core.h"
#include "entity_manager.h"
#include "light.h"

namespace lib_graphics {
class GlShadowMapping {
 public:
  GlShadowMapping(lib_core::EngineCore *engine);
  ~GlShadowMapping();

  void DrawShadowMap(std::pair<lib_core::Entity, Light> &light);

 protected:
 private:
  lib_core::EngineCore *engine_;

  ct::dyn_array<size_t> shader_ids_;

  size_t point_shadow_material_;
  size_t dir_shadow_material_;

  int light_pos_loc_ = -1;
  int far_plane_loc_ = -1;
};
}  // namespace lib_graphics
