#include "gl_skybox_shading.h"
#include <GL/glew.h>
#include "gl_material_system.h"
#include "mesh_system.h"
#include "skybox.h"

namespace lib_graphics {
GlSkyboxShading::GlSkyboxShading(lib_core::EngineCore *engine)
    : engine_(engine) {
  ct::string vert_shader =
      "#version 330 core\n"
      "layout(location = 0) in vec3 position;\n"
      "layout(location = 1) in vec3 normal;\n"
      "layout(location = 2) in vec3 tangent;\n"
      "layout(location = 3) in vec2 texcoord;\n\n"
      "out vec3 TexCoords;\n\n"

      "uniform mat4 projection;\n"
      "uniform mat4 view;\n\n"

      "void main()\n"
      "{\n"
      "  vec4 pos = projection * view * vec4(position, 1.0);\n"
      "  gl_Position = pos.xyww;\n"
      "  TexCoords = position.xyz;\n"
      "}";

  ct::string frag_shader =
      "#version 330 core\n"
      "in vec3 TexCoords;\n"
      "out vec4 color;\n\n"

      "uniform samplerCube skybox;\n\n"

      "void main()\n"
      "{\n"
      "  color = texture(skybox, TexCoords);\n"
      "}";

  auto shader_command = AddShaderCommand(vert_shader, frag_shader);
  issue_command(shader_command);
  skybox_material_.shader = shader_command.ShaderId();
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
