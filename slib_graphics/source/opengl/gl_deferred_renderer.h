#pragma once
#include "engine_core.h"
#include "gl_mesh_system.h"
#include "gl_particle_system.h"
#include "gui_renderer.h"
#include "material_system.h"
#include "matrix4x4.h"
#include "renderer.h"

namespace lib_graphics {
class GlDeferredRenderer : public Renderer {
 public:
  GlDeferredRenderer(lib_core::EngineCore* engine);
  ~GlDeferredRenderer() override;

  void RenderFrame(float dt) override;
  void InitRenderer() override;

  void Clear(lib_core::Vector4 color) override;

 private:
  struct QuadVert {
    float pos[2];
    float tex_coord[2];
  };

  lib_core::EngineCore* engine_;

  size_t frame_buffer_, hdr_buffer_;
  GlMeshSystem::MeshInfo full_screen_quad_;
  size_t deffered_ambient_material_, tonemap_gamma_material_;
  unsigned white_texture_, black_texture_;

  TextureDesc depth_desc_;
  int shader_locs_[20];

  std::unique_ptr<lib_gui::GuiRenderer> gui_renderer_;
  std::unique_ptr<class GlSsao> ssao_effect_;
  std::unique_ptr<class GlSmaa> smaa_effect_;
  std::unique_ptr<class GlBloom> bloom_effect_;
  std::unique_ptr<class GlGausianBlur> blur_effect_;
  std::unique_ptr<class GlSkyboxShading> skybox_effect_;
  std::unique_ptr<class GlDeferredShading> deferred_shading_effect_;
  std::unique_ptr<class GlDeferredLighting> deferred_lighting_effect_;
};
}  // namespace lib_graphics
