#include "gl_deferred_lighting.h"
#include <GL/glew.h>
#include "culling_system.h"
#include "gl_material_system.h"
#include "light_system.h"
#include "mesh_system.h"
#include "window.h"

namespace lib_graphics {
GlDeferredLighting::GlDeferredLighting(lib_core::EngineCore *engine,
                                       const TextureDesc &position_tex,
                                       const TextureDesc &normal_tex,
                                       const TextureDesc &albedo_tex,
                                       const TextureDesc &rme_tex,
                                       const TextureDesc &depth_tex)
    : engine_(engine) {
  ct::string vert_shader =
      cu::ReadFile("./content/shaders/opengl/deferred_lighting_world_vs.glsl");

  auto shader_command = AddShaderCommand(vert_shader);
  shader_ids_.push_back(shader_command.ShaderId());
  issue_command(shader_command);

  Material material;
  material.shader = shader_ids_.back();

  auto material_command = AddMaterialCommand(material);
  stencil_pass_ = material_command.MaterialId();
  issue_command(material_command);

  shader_command = AddShaderCommand(
      vert_shader,
      cu::ReadFile("./content/shaders/opengl/"
                   "deferred_lighting_point_shadow_world_fs.glsl"));
  shader_ids_.push_back(shader_command.ShaderId());
  issue_command(shader_command);

  material.shader = shader_ids_.back();
  material.textures.push_back(position_tex);
  material.textures.push_back(normal_tex);
  material.textures.push_back(albedo_tex);
  material.textures.push_back(rme_tex);
  material.textures.push_back(depth_tex);

  material_command = AddMaterialCommand(material);
  deferred_lighting_point_shadow_volume_ = material_command.MaterialId();
  issue_command(material_command);

  shader_command = AddShaderCommand(
      vert_shader,
      cu::ReadFile(
          "./content/shaders/opengl/deferred_lighting_point_world_fs.glsl"));
  shader_ids_.push_back(shader_command.ShaderId());
  issue_command(shader_command);

  material.shader = shader_ids_.back();
  material_command = AddMaterialCommand(material);
  deferred_lighting_volume_ = material_command.MaterialId();
  issue_command(material_command);

  vert_shader =
      cu::ReadFile("./content/shaders/opengl/deferred_lighting_screen_vs.glsl");

  shader_command = AddShaderCommand(
      vert_shader,
      cu::ReadFile("./content/shaders/opengl/"
                   "deferred_lighting_point_shadow_screen_fs.glsl"));
  shader_ids_.push_back(shader_command.ShaderId());
  issue_command(shader_command);

  material.shader = shader_ids_.back();
  material_command = AddMaterialCommand(material);
  deferred_lighting_point_shadow_quad_ = material_command.MaterialId();
  issue_command(material_command);

  shader_command = AddShaderCommand(
      vert_shader,
      cu::ReadFile(
          "./content/shaders/opengl/deferred_lighting_point_screen_fs.glsl"));
  shader_ids_.push_back(shader_command.ShaderId());
  issue_command(shader_command);

  material.shader = shader_ids_.back();
  material_command = AddMaterialCommand(material);
  deferred_lighting_quad_ = material_command.MaterialId();
  issue_command(material_command);

  shader_command = AddShaderCommand(
      vert_shader,
      cu::ReadFile("./content/shaders/opengl/"
                   "deferred_lighting_directional_shadow_fs.glsl"));
  shader_ids_.push_back(shader_command.ShaderId());
  issue_command(shader_command);

  material.shader = shader_ids_.back();
  material_command = AddMaterialCommand(material);
  deferred_lighting_dir_shadow_quad_ = material_command.MaterialId();
  issue_command(material_command);

  shader_command = AddShaderCommand(
      vert_shader,
      cu::ReadFile(
          "./content/shaders/opengl/deferred_lighting_directional_fs.glsl"));
  shader_ids_.push_back(shader_command.ShaderId());
  issue_command(shader_command);

  material.shader = shader_ids_.back();
  material_command = AddMaterialCommand(material);
  deferred_lighting_dir_quad_ = material_command.MaterialId();
  issue_command(material_command);

  shadow_mapper_ = std::make_unique<GlShadowMapping>(engine_);
}

GlDeferredLighting::~GlDeferredLighting() {
  for (auto &id : shader_ids_) issue_command(RemoveShaderCommand(id));

  issue_command(RemoveMaterialCommand(deferred_lighting_point_shadow_volume_));
  issue_command(RemoveMaterialCommand(deferred_lighting_point_shadow_quad_));
  issue_command(RemoveMaterialCommand(deferred_lighting_dir_shadow_quad_));
  issue_command(RemoveMaterialCommand(deferred_lighting_dir_quad_));
  issue_command(RemoveMaterialCommand(deferred_lighting_volume_));
  issue_command(RemoveMaterialCommand(deferred_lighting_quad_));
  issue_command(RemoveMaterialCommand(stencil_pass_));
}

void GlDeferredLighting::DrawLights(Camera &cam, lib_core::Entity cam_entity,
                                    lib_core::Vector3 cam_pos) {
  auto lighting_timer = cu::TimerStart();
  auto cull_system = engine_->GetCulling();

  auto &light_matrices = cull_system->GetLightMatrices(cam_entity);
  auto lights = cull_system->GetLightPacks(cam_entity);

  light_mats.clear();
  light_packs.clear();
  int light_matrix_id = 0;
  for (auto light_ent : *lights) {
    auto light = g_ent_mgr.GetOldCbeR<Light>(light_ent);
    if (!light) continue;

    size_t material;
    if (light->type == Light::kPoint) {
      auto cam_d = lib_core::Vector3(cam_pos[0], cam_pos[1], cam_pos[2]);
      cam_d -= light->data_pos;
      auto cam_dist = cam_d.Length();
      bool inside = light->max_radius + 1.0f > cam_dist;

      material =
          light->cast_shadows
              ? (inside ? deferred_lighting_point_shadow_quad_
                        : deferred_lighting_point_shadow_volume_)
              : (inside ? deferred_lighting_quad_ : deferred_lighting_volume_);
      light_mats[material].push_back(light_matrices[light_matrix_id++]);
    } else if (light->type == Light::kDir) {
      material = light->cast_shadows ? deferred_lighting_dir_shadow_quad_
                                     : deferred_lighting_dir_quad_;
      light_mats[material].push_back(light_matrices[light_matrix_id++]);
      light_mats[material].push_back(light_matrices[light_matrix_id++]);
      light_mats[material].push_back(light_matrices[light_matrix_id++]);
    }
    light_packs[material].push_back({light_ent, *light});
  }

  InstanceDraw(cam, cam_entity, cam_pos);
  engine_->GetDebugOutput()->UpdateBottomRightLine(
      2, std::to_string(cu::TimerStop<std::milli>(lighting_timer)) +
             " :Lighting time");
}

void GlDeferredLighting::SetScreenQuad(unsigned quad) { screen_quad_ = quad; }

void GlDeferredLighting::StencilDraw(Camera &cam, lib_core::Entity cam_entity,
                                     lib_core::Vector3 cam_pos) {
  auto mat_system = static_cast<GlMaterialSystem *>(engine_->GetMaterial());
  auto mesh_system = engine_->GetMesh();
  auto window = engine_->GetWindow();

  glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
  glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
  for (auto &l_pack : light_packs) {
    int inst_count = 0;
    int max_instance =
        1;  // l_pack.second[0].second.type == Light::kDir ? 5 : 50;
    ct::dyn_array<std::pair<lib_core::Entity, Light>> light_pack;
    for (int iii = 0; iii < l_pack.second.size(); iii += max_instance) {
      light_pack.clear();
      for (int ii = iii; ii < l_pack.second.size(); ++ii) {
        light_pack.push_back(l_pack.second[ii]);
        if (light_pack.size() == max_instance) break;
      }

      // Render shadows
      if (l_pack.first == deferred_lighting_dir_shadow_quad_ ||
          l_pack.first == deferred_lighting_point_shadow_volume_ ||
          l_pack.first == deferred_lighting_point_shadow_quad_) {
        glDepthMask(GL_TRUE);
        glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_STENCIL_TEST);
        shadow_mapper_->DrawShadowMap(light_pack[0]);
      }

      glDepthMask(GL_FALSE);
      auto &mats = light_mats[l_pack.first];
      if (l_pack.first != deferred_lighting_dir_shadow_quad_ &&
          l_pack.first != deferred_lighting_dir_quad_) {
        glEnable(GL_STENCIL_TEST);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glStencilFunc(GL_ALWAYS, 0, 0xFF);

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glClear(GL_STENCIL_BUFFER_BIT);

        mat_system->ApplyMaterial(stencil_pass_, nullptr, true);

        auto shader_program = mat_system->GetCurrentShader();
        if (stencil_vp_loc_ - 1)
          stencil_vp_loc_ = glGetUniformLocation(shader_program, "view_proj");
        glUniformMatrix4fv(stencil_vp_loc_, 1, GL_FALSE, cam.view_proj_.data);

        if (stencil_world_loc_ == -1)
          stencil_world_loc_ = glGetUniformLocation(shader_program, "world[0]");
        glUniformMatrix4fv(stencil_world_loc_, int(light_pack.size()), GL_FALSE,
                           mats[inst_count].data);

        mesh_system->DrawMesh(lib_core::EngineCore::stock_sphere_mesh,
                              int(light_pack.size()), true);
      }

      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glDisable(GL_DEPTH_TEST);
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE);
      glEnable(GL_CULL_FACE);

      if (l_pack.first != deferred_lighting_dir_shadow_quad_ &&
          l_pack.first != deferred_lighting_dir_quad_) {
        if (l_pack.first == deferred_lighting_quad_ ||
            l_pack.first == deferred_lighting_point_shadow_quad_) {
          glCullFace(GL_BACK);
          glStencilFunc(GL_LESS, 0, 0xFF);
        } else {
          glCullFace(GL_FRONT);
          glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
        }
      } else {
        glCullFace(GL_BACK);
        glDisable(GL_STENCIL_TEST);
      }

      mat_system->ApplyMaterial(l_pack.first, &light_pack, true);
      auto shader_program = mat_system->GetCurrentShader();

      auto loc_it = shader_locations_.find(shader_program);
      if (loc_it == shader_locations_.end()) {
        auto &data = shader_locations_[shader_program];
        data[0] = glGetUniformLocation(shader_program, "cam_pos");
        data[1] = glGetUniformLocation(shader_program, "screen_dim");
        data[2] = glGetUniformLocation(shader_program, "view_proj");
        data[3] = glGetUniformLocation(shader_program, "world[0]");
        loc_it = shader_locations_.find(shader_program);
      }

      std::array<float, 2> scr_dim = {float(window->GetRenderDim().first),
                                      float(window->GetRenderDim().second)};

      glUniform3fv(loc_it->second[0], 1, &cam_pos[0]);
      glUniform2fv(loc_it->second[1], 1, scr_dim.data());
      glUniformMatrix4fv(loc_it->second[2], 1, GL_FALSE, cam.view_proj_.data);
      if (l_pack.first == deferred_lighting_dir_shadow_quad_)
        glUniformMatrix4fv(loc_it->second[3], 3, GL_FALSE,
                           mats[inst_count].data);
      else
        glUniformMatrix4fv(loc_it->second[3], int(light_pack.size()), GL_FALSE,
                           mats[inst_count].data);

      inst_count += int(light_pack.size());

      if (l_pack.first == deferred_lighting_point_shadow_quad_ ||
          l_pack.first == deferred_lighting_quad_ ||
          l_pack.first == deferred_lighting_dir_shadow_quad_ ||
          l_pack.first == deferred_lighting_dir_quad_) {
        glBindVertexArray(screen_quad_);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 3, int(light_pack.size()));
      } else
        mesh_system->DrawMesh(lib_core::EngineCore::stock_sphere_mesh,
                              int(light_pack.size()), true);
    }
  }

  glDisable(GL_STENCIL_TEST);
  glEnable(GL_DEPTH_TEST);
  glCullFace(GL_BACK);
  glDisable(GL_BLEND);
  glDepthMask(GL_TRUE);
}

void GlDeferredLighting::InstanceDraw(Camera &cam, lib_core::Entity cam_entity,
                                      lib_core::Vector3 cam_pos) {
  auto mat_system = engine_->GetMaterial();
  auto mesh_system = engine_->GetMesh();
  auto window = engine_->GetWindow();

  for (auto &l_pack : light_packs) {
    int inst_count = 0;
    int max_instance = l_pack.second[0].second.type == Light::kDir ? 5 : 50;
    if (l_pack.first == deferred_lighting_dir_shadow_quad_ ||
        l_pack.first == deferred_lighting_point_shadow_volume_ ||
        l_pack.first == deferred_lighting_point_shadow_quad_)
      max_instance = 1;

    ct::dyn_array<std::pair<lib_core::Entity, Light>> light_pack;
    for (int iii = 0; iii < l_pack.second.size(); iii += max_instance) {
      light_pack.clear();
      for (int ii = iii; ii < l_pack.second.size(); ++ii) {
        light_pack.push_back(l_pack.second[ii]);
        if (light_pack.size() == max_instance) break;
      }

      // Render shadows
      if (l_pack.first == deferred_lighting_dir_shadow_quad_ ||
          l_pack.first == deferred_lighting_point_shadow_volume_ ||
          l_pack.first == deferred_lighting_point_shadow_quad_) {
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        shadow_mapper_->DrawShadowMap(light_pack[0]);
      }

      glDepthMask(GL_FALSE);
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE);

      auto &mats = light_mats[l_pack.first];
      mat_system->ApplyMaterial(l_pack.first, &light_pack, true);
      auto shader_program = mat_system->GetCurrentShader();

      auto loc_it = shader_locations_.find(shader_program);
      if (loc_it == shader_locations_.end()) {
        auto &data = shader_locations_[shader_program];
        data[0] = glGetUniformLocation(shader_program, "cam_pos");
        data[1] = glGetUniformLocation(shader_program, "screen_dim");
        data[2] = glGetUniformLocation(shader_program, "view_proj");
        data[3] = glGetUniformLocation(shader_program, "world[0]");
        loc_it = shader_locations_.find(shader_program);
      }

      std::array<float, 2> scr_dim = {float(window->GetRenderDim().first),
                                      float(window->GetRenderDim().second)};

      glUniform3fv(loc_it->second[0], 1, &cam_pos[0]);
      glUniform2fv(loc_it->second[1], 1, scr_dim.data());
      glUniformMatrix4fv(loc_it->second[2], 1, GL_FALSE, cam.view_proj_.data);
      if (l_pack.first == deferred_lighting_dir_shadow_quad_)
        glUniformMatrix4fv(loc_it->second[3], 3, GL_FALSE,
                           mats[inst_count].data);
      else
        glUniformMatrix4fv(loc_it->second[3], int(light_pack.size()), GL_FALSE,
                           mats[inst_count].data);

      inst_count += int(light_pack.size());

      if (l_pack.first == deferred_lighting_point_shadow_quad_ ||
          l_pack.first == deferred_lighting_quad_ ||
          l_pack.first == deferred_lighting_dir_shadow_quad_ ||
          l_pack.first == deferred_lighting_dir_quad_) {
        glBindVertexArray(screen_quad_);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 3, int(light_pack.size()));
      } else
        mesh_system->DrawMesh(lib_core::EngineCore::stock_sphere_mesh,
                              int(light_pack.size()), true);
    }
  }

  glDisable(GL_BLEND);
  glDepthMask(GL_TRUE);
}
}  // namespace lib_graphics
