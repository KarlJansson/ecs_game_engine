#include "gl_renderer.h"
#include <GL/glew.h>
#include "camera.h"
#include "culling_system.h"
#include "entity_manager.h"
#include "gl_camera_system.h"
#include "gl_shadow_mapping.h"
#include "gl_skybox_shading.h"
#include "light.h"
#include "skybox.h"
#include "system_manager.h"
#include "text_system.h"

namespace lib_graphics {
GlRenderer::GlRenderer(lib_core::EngineCore *engine) : engine_(engine) {
  static_cast<GlMaterialSystem *>(engine_->GetMaterial())->CompileShaders();
}

void GlRenderer::InitRenderer() {
  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) return;

  // glEnable(GL_FRAMEBUFFER_SRGB);
  glEnable(GL_MULTISAMPLE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  auto dim = engine_->GetWindow()->GetRenderDim();
  glViewport(0, 0, dim.first, dim.second);

  shadow_mapping_effect_ = std::make_unique<GlShadowMapping>(engine_);
  skybox_effect_ = std::make_unique<GlSkyboxShading>(engine_);
}

void GlRenderer::Clear(lib_core::Vector4 color) {
  glClearColor(color[0], color[1], color[2], color[3]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GlRenderer::RenderFrame(float dt) {
  auto cull_system = engine_->GetCulling();
  auto mat_system = static_cast<GlMaterialSystem *>(engine_->GetMaterial());
  auto mesh_system = engine_->GetMesh();

  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  auto &world_matrices = cull_system->GetWorldMatrices();
  auto &world_inv_trans_matrices = cull_system->GetWorldInvTransMatrices();

  auto cam_entities = g_ent_mgr.GetEbt<Camera>();
  auto camera_comps = g_ent_mgr.GetOldCbt<Camera>();

  // shadow_mapping_effect_->DrawShadowMaps();
  mat_system->PopFrameBuffer();

  if (camera_comps) {
    for (int i = 0; i < camera_comps->size(); ++i) {
      auto mesh_packs = cull_system->GetMeshPacks(cam_entities->at(i));
      auto &cam = camera_comps->at(i);

      for (auto &pack : *mesh_packs) {
        // mat_system->ApplyMaterial(pack.material_id, &lights);
        auto shader_id = mat_system->GetCurrentShader();
        GLint cam_loc = glGetUniformLocation(shader_id, "cam_pos");
        GLint view_loc = glGetUniformLocation(shader_id, "view");
        GLint proj_loc = glGetUniformLocation(shader_id, "projection");
        if (cam_loc != -1) glUniform3fv(cam_loc, 1, (float *)&cam.position_);
        if (view_loc != -1)
          glUniformMatrix4fv(view_loc, 1, GL_FALSE, cam.view_.data);
        if (proj_loc != -1)
          glUniformMatrix4fv(proj_loc, 1, GL_FALSE, cam.proj_.data);

        GLint world_loc = glGetUniformLocation(shader_id, "world[0]");
        GLint world_inv_trans_loc =
            glGetUniformLocation(shader_id, "world_inv_trans[0]");

        auto count = int(pack.mesh_count);
        while (count > 0) {
          glUniformMatrix4fv(
              world_loc, count > 50 ? 50 : count, GL_FALSE,
              (float *)&world_matrices[pack.start_ind +
                                       (pack.mesh_count - count)]);
          glUniformMatrix4fv(
              world_inv_trans_loc, count > 50 ? 50 : count, GL_FALSE,
              (float *)&world_inv_trans_matrices[pack.start_ind +
                                                 (pack.mesh_count - count)]);

          mesh_system->DrawMesh(pack.mesh_id, count > 50 ? 50 : count);
          count -= 50;
        }
      }

      skybox_effect_->DrawSkybox(cam, cam_entities->at(i));
    }
  }
}
}  // namespace lib_graphics
