#include "gl_shadow_mapping.h"
#include <GL/glew.h>
#include "culling_system.h"
#include "gl_material_system.h"
#include "light_system.h"
#include "mesh_system.h"
#include "window.h"

namespace lib_graphics {
GlShadowMapping::GlShadowMapping(lib_core::EngineCore *engine)
    : engine_(engine) {
  ct::string vertex_header_ =
      "#version 430 core\n"
      "layout(location = 0) in vec3 position;\n"
      "layout(location = 1) in vec3 normal;\n"
      "layout(location = 2) in vec3 tangent;\n"
      "layout(location = 3) in vec2 texcoord;\n\n";

  ct::string vert_shader =
      vertex_header_ +

      "uniform mat4 world[200];\n\n"

      "void main()\n"
      "{\n"
      "  gl_Position = world[gl_InstanceID] * vec4(position, 1.0);\n"
      "}";

  ct::string frag_shader =
      "#version 430 core\n"
      "in vec4 FragPos;\n\n"

      "uniform vec3 light_pos;\n"
      "uniform float far_plane;\n\n"

      "void main()\n"
      "{\n"
      "  float lightDistance = length(FragPos.xyz - light_pos);\n"
      "  lightDistance = lightDistance / far_plane;\n"
      "  gl_FragDepth = lightDistance;\n"
      "}";

  ct::string geom_shader =
      "#version 430 core\n"
      "layout(triangles) in;\n"
      "layout(triangle_strip, max_vertices = 18) out;\n\n"

      "uniform mat4 shadow_matrices[6];\n\n"

      "out vec4 FragPos;\n\n"

      "void main()\n"
      "{\n"
      "  for (int face = 0; face < 6; ++face)\n"
      "  {\n"
      "    gl_Layer = face;\n"
      "    for (int i = 0; i < 3; ++i)\n"
      "    {\n"
      "      FragPos = gl_in[i].gl_Position;\n"
      "      gl_Position = shadow_matrices[face] * FragPos;\n"
      "      EmitVertex();\n"
      "    }\n"
      "    EndPrimitive();\n"
      "  }\n"
      "}";

  Material material;

  auto shader_command = AddShaderCommand(vert_shader, frag_shader, geom_shader);
  issue_command(shader_command);
  shader_ids_.push_back(shader_command.ShaderId());
  material.shader = shader_command.ShaderId();

  auto material_command = AddMaterialCommand(material);
  issue_command(material_command);
  point_shadow_material_ = material_command.MaterialId();

  vert_shader = vertex_header_ +

                "uniform mat4 shadow_matrices[3];\n"
                "uniform mat4 world[200];\n"

                "void main()\n"
                "{\n"
                "  gl_Position = shadow_matrices[0] * (world[gl_InstanceID] * "
                "vec4(position, 1.0f));\n"
                "}";

  frag_shader =
      "#version 430 core\n"

      "void main()\n"
      "{\n"
      "  gl_FragDepth = gl_FragCoord.z;\n"
      "}\n";

  shader_command = AddShaderCommand(vert_shader, frag_shader);
  issue_command(shader_command);
  shader_ids_.push_back(shader_command.ShaderId());
  material.shader = shader_command.ShaderId();

  material_command = AddMaterialCommand(material);
  issue_command(material_command);
  dir_shadow_material_ = material_command.MaterialId();
}

GlShadowMapping::~GlShadowMapping() {
  for (auto &id : shader_ids_) issue_command(RemoveShaderCommand(id));
  issue_command(RemoveMaterialCommand(point_shadow_material_));
  issue_command(RemoveMaterialCommand(dir_shadow_material_));
}

void GlShadowMapping::DrawShadowMap(std::pair<lib_core::Entity, Light> &light) {
  auto cull_system = engine_->GetCulling();
  auto mat_system = static_cast<GlMaterialSystem *>(engine_->GetMaterial());
  auto mesh_system = engine_->GetMesh();
  auto light_mesh_packs = cull_system->GetMeshPacks(light.first);

  auto &world_matrices = cull_system->GetWorldMatrices();
  size_t frame_buff;

  ct::dyn_array<lib_core::Matrix4x4> shadow_transforms =
      LightSystem::GetShadowMatrices(light.second, false);

  size_t nr_maps = 1;
  GLuint shader_id = 0;
  switch (light.second.type) {
    case Light::kPoint:
      mat_system->ApplyMaterial(point_shadow_material_);
      shader_id = mat_system->GetCurrentShader();

      if (light_pos_loc_ == -1)
        light_pos_loc_ = glGetUniformLocation(shader_id, "light_pos");
      if (far_plane_loc_ == -1)
        far_plane_loc_ = glGetUniformLocation(shader_id, "far_plane");

      glUniform3fv(light_pos_loc_, 1, (float *)&light.second.data_pos);
      glUniform1f(far_plane_loc_, light.second.max_radius);

      frame_buff = mat_system->Get3DShadowFrameBuffer(
          light.second.shadow_resolutions[0]);
      break;
    case Light::kDir:
      mat_system->ApplyMaterial(dir_shadow_material_);
      shader_id = mat_system->GetCurrentShader();
      nr_maps = shadow_transforms.size();
      break;
    default:
      cu::AssertWarning(false, "Faulty light type specified.", __FILE__,
                        __LINE__);
      return;
  }

  int max_inst = 200;
  GLint world_loc = glGetUniformLocation(shader_id, "world[0]");
  GLint loc = glGetUniformLocation(shader_id, "shadow_matrices[0]");
  for (int i = 0; i < nr_maps; ++i) {
    if (light.second.type == Light::kDir) {
      frame_buff = mat_system->Get2DShadowFrameBuffer(
          light.second.shadow_resolutions[i]);
      glUniformMatrix4fv(loc, 1, GL_FALSE, shadow_transforms[i].data);
    } else if (light.second.type == Light::kPoint) {
      glUniformMatrix4fv(loc, GLsizei(shadow_transforms.size()), GL_FALSE,
                         shadow_transforms[0].data);
    }

    mat_system->PushFrameBuffer(frame_buff);
    glClear(GL_DEPTH_BUFFER_BIT);

    for (auto &pack : *light_mesh_packs) {
      int count = int(pack.mesh_count);
      while (count > 0) {
        if (world_loc != -1)
          glUniformMatrix4fv(
              world_loc, count > max_inst ? max_inst : count, GL_FALSE,
              (float *)&world_matrices[pack.start_ind +
                                       (pack.mesh_count - count)]);

        mesh_system->DrawMesh(pack.mesh_id, count > max_inst ? max_inst : count,
                              true);
        count -= max_inst;
      }
    }

    mat_system->PopFrameBuffer();
  }
}
}  // namespace lib_graphics
