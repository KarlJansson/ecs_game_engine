#pragma once
#include "camera.h"
#include "engine_core.h"
#include "entity.h"
#include "gl_shadow_mapping.h"
#include "light.h"
#include "material_system.h"
#include "matrix4x4.h"

namespace lib_graphics {
class GlDeferredLighting {
 public:
  GlDeferredLighting(lib_core::EngineCore* engine,
                     const TextureDesc& position_tex,
                     const TextureDesc& normal_tex,
                     const TextureDesc& albedo_tex, const TextureDesc& rme_tex,
                     const TextureDesc& depth_tex);
  ~GlDeferredLighting();

  void DrawLights(Camera &cam, lib_core::Entity cam_entity, float cam_pos[3]);
  void SetScreenQuad(unsigned quad);

 protected:
 private:
  void StencilDraw(Camera &cam, lib_core::Entity cam_entity, float cam_pos[3]);
  void InstanceDraw(Camera &cam, lib_core::Entity cam_entity, float cam_pos[3]);

  lib_core::EngineCore *engine_;
  ct::dyn_array<size_t> shader_ids_;

  unsigned screen_quad_;

  int stencil_vp_loc_ = -1, stencil_world_loc_ = -1;
  ct::hash_map<unsigned, int[4]> shader_locations_;
  std::unique_ptr<GlShadowMapping> shadow_mapper_;

  ct::hash_map<size_t, ct::dyn_array<std::pair<lib_core::Entity, Light>>>
      light_packs;
  ct::hash_map<size_t, ct::dyn_array<lib_core::Matrix4x4>> light_mats;

  size_t deferred_lighting_point_shadow_volume_;
  size_t deferred_lighting_point_shadow_quad_;
  size_t deferred_lighting_dir_shadow_quad_;
  size_t deferred_lighting_dir_quad_;
  size_t deferred_lighting_volume_;
  size_t deferred_lighting_quad_;

  size_t stencil_pass_;
};
}  // namespace lib_graphics
