#pragma once
#include "engine_core.h"
#include "gl_material_system.h"
#include "gl_mesh_system.h"
#include "gl_text_system.h"
#include "gl_window.h"
#include "renderer.h"
#include "gl_shadow_mapping.h"
#include "gl_skybox_shading.h"

namespace lib_graphics {
class GlRenderer : public Renderer {
 public:
  GlRenderer(lib_core::EngineCore* engine);
  ~GlRenderer() override = default;

  void RenderFrame(float dt) override;
  void InitRenderer() override;

  void Clear(lib_core::Vector4 color) override;

 protected:
 private:
  lib_core::EngineCore* engine_;

  std::unique_ptr<GlShadowMapping> shadow_mapping_effect_;
  std::unique_ptr<GlSkyboxShading> skybox_effect_;
};
}  // namespace lib_graphics
