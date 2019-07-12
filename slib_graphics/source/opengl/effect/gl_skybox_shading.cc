#include "gl_skybox_shading.h"
#include <GL/glew.h>
#include "gl_material_system.h"
#include "mesh_system.h"
#include "skybox.h"

namespace lib_graphics {
GlSkyboxShading::GlSkyboxShading(lib_core::EngineCore *engine)
    : engine_(engine) {
  auto shader_command =
      AddShaderCommand(cu::ReadFile("./content/shaders/opengl/skybox_vs.glsl"),
                       cu::ReadFile("./content/shaders/opengl/skybox_fs.glsl"));
  skybox_material_.shader = shader_command.ShaderId();
  issue_command(shader_command);
}

GlSkyboxShading::~GlSkyboxShading() {
  issue_command(RemoveShaderCommand(skybox_material_.shader));
}

void GlSkyboxShading::DrawSkybox(Camera &cam, lib_core::Entity cam_ent) {
  auto mat_system = static_cast<GlMaterialSystem *>(engine_->GetMaterial());
  auto mesh_system = engine_->GetMesh();

  auto skybox = g_ent_mgr.GetOldCbeR<Skybox>(cam_ent);
  if (skybox) {
    skybox_material_.textures.clear();
    skybox_material_.textures.push_back({skybox->texture_, "skybox"});
    if (mat_system->ForceMaterial(skybox_material_)) {
      auto shader_id = mat_system->GetCurrentShader();
      if (view_loc_ == -1) view_loc_ = glGetUniformLocation(shader_id, "view");
      if (proj_loc_ == -1)
        proj_loc_ = glGetUniformLocation(shader_id, "projection");

      auto view_mod = cam.view_;
      view_mod.data[3] = view_mod.data[7] = view_mod.data[11] =
          view_mod.data[12] = view_mod.data[13] = view_mod.data[14] =
              view_mod.data[15] = 0;
      glUniformMatrix4fv(view_loc_, 1, GL_FALSE, view_mod.data);
      glUniformMatrix4fv(proj_loc_, 1, GL_FALSE, cam.proj_.data);

      glDepthFunc(GL_LEQUAL);
      glCullFace(GL_FRONT);
      mesh_system->DrawMesh(lib_core::EngineCore::stock_box_mesh, 1, true);
      glCullFace(GL_BACK);
      glDepthFunc(GL_LESS);
    }
  }
}
}  // namespace lib_graphics
