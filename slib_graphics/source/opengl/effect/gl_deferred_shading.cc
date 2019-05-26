#include "gl_deferred_shading.h"
#include <GL/glew.h>
#include "culling_system.h"
#include "gl_material_system.h"
#include "mesh_system.h"

namespace lib_graphics {
GlDeferredShading::GlDeferredShading(lib_core::EngineCore *engine)
    : engine_(engine) {
  max_inst_ = 100;
}

void GlDeferredShading::DrawGBuffers(
    const Camera cam,
    const ct::dyn_array<CullingSystem::MeshPack> &mesh_packs) {
  auto gbuffer_timer = cu::TimerStart();
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Draw Gbuffers",
                  __FILE__, __LINE__);

  auto cull_system = engine_->GetCulling();
  auto mat_system = static_cast<GlMaterialSystem *>(engine_->GetMaterial());
  auto mesh_system = engine_->GetMesh();

  auto &world_matrices = cull_system->GetWorldMatrices();
  auto &world_inv_trans_matrices = cull_system->GetWorldInvTransMatrices();
  auto &albedo_vecs = cull_system->GetAlbedoVecs();
  auto &rme_vecs = cull_system->GetRmeVecs();
  auto &tex_scale = cull_system->GetTexScaleVecs();
  auto &tex_offset = cull_system->GetTexOffsetVecs();

  draw_calls_ = 0;
  for (auto &pack : mesh_packs) {
    cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Draw Gbuffers",
                    __FILE__, __LINE__);

    mat_system->ApplyMaterial(pack.material_id);

    cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Draw Gbuffers",
                    __FILE__, __LINE__);

    auto shader_id = mat_system->GetCurrentShader();
    auto it = shader_locations_.find(shader_id);
    if (it == shader_locations_.end()) {
      auto &data = shader_locations_[shader_id];
      data[0] = glGetUniformLocation(shader_id, "cam_pos");
      data[1] = glGetUniformLocation(shader_id, "view_proj");
      data[2] = glGetUniformLocation(shader_id, "world[0]");
      data[3] = glGetUniformLocation(shader_id, "world_inv_trans[0]");
      data[4] = glGetUniformLocation(shader_id, "albedo[0]");
      data[5] = glGetUniformLocation(shader_id, "rme[0]");
      data[6] = glGetUniformLocation(shader_id, "tex_scale[0]");
      data[7] = glGetUniformLocation(shader_id, "tex_offset[0]");
      it = shader_locations_.find(shader_id);
    }

    cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Draw Gbuffers",
                    __FILE__, __LINE__);

    if (it->second[0] != -1)
      glUniform3fv(it->second[0], 1, cam.position_.data());
    if (it->second[1] != -1)
      glUniformMatrix4fv(it->second[1], 1, GL_FALSE, cam.view_proj_.data);

    cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Draw Gbuffers",
                    __FILE__, __LINE__);

    auto count = int(pack.mesh_count);
    while (count > 0) {
      glUniformMatrix4fv(
          it->second[2], count > max_inst_ ? max_inst_ : count, GL_FALSE,
          world_matrices[pack.start_ind + (pack.mesh_count - count)].data);
      glUniformMatrix4fv(
          it->second[3], count > max_inst_ ? max_inst_ : count, GL_FALSE,
          world_inv_trans_matrices[pack.start_ind + (pack.mesh_count - count)]
              .data);
      glUniform3fv(
          it->second[4], count > max_inst_ ? max_inst_ : count,
          albedo_vecs[pack.start_ind + (pack.mesh_count - count)].data());
      glUniform3fv(it->second[5], count > max_inst_ ? max_inst_ : count,
                   rme_vecs[pack.start_ind + (pack.mesh_count - count)].data());
      if (it->second[6])
        glUniform2fv(
            it->second[6], count > max_inst_ ? max_inst_ : count,
            tex_scale[pack.start_ind + (pack.mesh_count - count)].data());
      if (it->second[7])
        glUniform2fv(
            it->second[7], count > max_inst_ ? max_inst_ : count,
            tex_offset[pack.start_ind + (pack.mesh_count - count)].data());

      mesh_system->DrawMesh(pack.mesh_id,
                            count > max_inst_ ? max_inst_ : count);

      ++draw_calls_;
      count -= max_inst_;
    }
  }

  if (engine_->GetDebugOutput())
    engine_->GetDebugOutput()->UpdateBottomLeftLine(
        5, "GBuffer Draw Calls: " + std::to_string(draw_calls_));

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Draw Gbuffers",
                  __FILE__, __LINE__);
  engine_->GetDebugOutput()->UpdateBottomRightLine(
      3, std::to_string(cu::TimerStop<std::milli>(gbuffer_timer)) +
             " :Gbuffer time");
}

void GlDeferredShading::DrawTranslucents(
    const Camera cam,
    const ct::dyn_array<CullingSystem::MeshPack> &mesh_packs) {
  auto transl_timer = cu::TimerStart();
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Draw Gbuffers",
                  __FILE__, __LINE__);

  auto cull_system = engine_->GetCulling();
  auto mat_system = static_cast<GlMaterialSystem *>(engine_->GetMaterial());
  auto mesh_system = engine_->GetMesh();

  auto &world_matrices = cull_system->GetWorldMatrices(false);
  auto &world_inv_trans_matrices = cull_system->GetWorldInvTransMatrices(false);
  auto &albedo_vecs = cull_system->GetAlbedoVecs(false);
  auto &rme_vecs = cull_system->GetRmeVecs(false);
  auto &tex_scale = cull_system->GetTexScaleVecs(false);
  auto &tex_offset = cull_system->GetTexOffsetVecs(false);
  auto &transparency = cull_system->GetTransparencyVecs();

  draw_calls_ = 0;
  for (auto &pack : mesh_packs) {
    cu::AssertError(glGetError() == GL_NO_ERROR,
                    "OpenGL error - Draw translucent", __FILE__, __LINE__);

    Material material = mat_system->GetMaterial(pack.material_id);
    if (material.shader == 0) continue;

    material.shader = mat_system->GetStockShaderIdConvert(material.shader);
    mat_system->ForceMaterial(material);

    cu::AssertError(glGetError() == GL_NO_ERROR,
                    "OpenGL error - Draw translucent", __FILE__, __LINE__);

    auto shader_id = mat_system->GetCurrentShader();
    auto it = shader_locations_.find(shader_id);
    if (it == shader_locations_.end()) {
      auto &data = shader_locations_[shader_id];
      data[0] = glGetUniformLocation(shader_id, "cam_pos");
      data[1] = glGetUniformLocation(shader_id, "view_proj");
      data[2] = glGetUniformLocation(shader_id, "world[0]");
      data[3] = glGetUniformLocation(shader_id, "world_inv_trans[0]");
      data[4] = glGetUniformLocation(shader_id, "albedo[0]");
      data[5] = glGetUniformLocation(shader_id, "rme[0]");
      data[6] = glGetUniformLocation(shader_id, "tex_scale[0]");
      data[7] = glGetUniformLocation(shader_id, "tex_offset[0]");
      data[8] = glGetUniformLocation(shader_id, "transp[0]");
      it = shader_locations_.find(shader_id);
    }

    cu::AssertError(glGetError() == GL_NO_ERROR,
                    "OpenGL error - Draw translucent", __FILE__, __LINE__);

    if (it->second[0] != -1)
      glUniform3fv(it->second[0], 1, cam.position_.data());
    if (it->second[1] != -1)
      glUniformMatrix4fv(it->second[1], 1, GL_FALSE, cam.view_proj_.data);

    cu::AssertError(glGetError() == GL_NO_ERROR,
                    "OpenGL error - Draw translucent", __FILE__, __LINE__);

    auto count = int(pack.mesh_count);
    while (count > 0) {
      glUniformMatrix4fv(
          it->second[2], count > max_inst_ ? max_inst_ : count, GL_FALSE,
          world_matrices[pack.start_ind + (pack.mesh_count - count)].data);
      glUniformMatrix4fv(
          it->second[3], count > max_inst_ ? max_inst_ : count, GL_FALSE,
          world_inv_trans_matrices[pack.start_ind + (pack.mesh_count - count)]
              .data);
      glUniform3fv(
          it->second[4], count > max_inst_ ? max_inst_ : count,
          albedo_vecs[pack.start_ind + (pack.mesh_count - count)].data());
      glUniform3fv(it->second[5], count > max_inst_ ? max_inst_ : count,
                   rme_vecs[pack.start_ind + (pack.mesh_count - count)].data());
      if (it->second[6])
        glUniform2fv(
            it->second[6], count > max_inst_ ? max_inst_ : count,
            tex_scale[pack.start_ind + (pack.mesh_count - count)].data());
      if (it->second[7])
        glUniform2fv(
            it->second[7], count > max_inst_ ? max_inst_ : count,
            tex_offset[pack.start_ind + (pack.mesh_count - count)].data());
      if (it->second[8])
        glUniform1fv(it->second[8], count > max_inst_ ? max_inst_ : count,
                     &transparency[pack.start_ind + (pack.mesh_count - count)]);

      mesh_system->DrawMesh(pack.mesh_id, count > max_inst_ ? max_inst_ : count,
                            true);
      auto err = glGetError();
      cu::AssertError(err == GL_NO_ERROR,
                      "OpenGL error - Draw translucent: " + std::to_string(err),
                      __FILE__, __LINE__);

      ++draw_calls_;
      count -= max_inst_;
    }
  }

  glDisable(GL_BLEND);
  glDepthMask(GL_TRUE);
  engine_->GetDebugOutput()->UpdateBottomRightLine(
      7, std::to_string(cu::TimerStop<std::milli>(transl_timer)) +
             " :Translucency time");
  engine_->GetDebugOutput()->UpdateBottomLeftLine(
      7, "Translucency draw calls: " + std::to_string(draw_calls_));
}
}  // namespace lib_graphics
