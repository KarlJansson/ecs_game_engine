#include "gl_deferred_renderer.h"
#include <GL/glew.h>
#include "camera.h"
#include "core_commands.h"
#include "engine_settings.h"
#include "entity_manager.h"
#include "gl_bloom.h"
#include "gl_camera_system.h"
#include "gl_deferred_lighting.h"
#include "gl_deferred_shading.h"
#include "gl_gausian_blur.h"
#include "gl_renderer.h"
#include "gl_shadow_mapping.h"
#include "gl_skybox_shading.h"
#include "gl_smaa.h"
#include "gl_ssao.h"
#include "gui_factory.h"
#include "light.h"
#include "material_system.h"
#include "skybox.h"

namespace lib_graphics {
GlDeferredRenderer::GlDeferredRenderer(lib_core::EngineCore *engine)
    : engine_(engine) {
  static_cast<GlMaterialSystem *>(engine_->GetMaterial())->CompileShaders();
  for (int i = 0; i < 20; ++i) shader_locs_[i] = -1;
}

GlDeferredRenderer::~GlDeferredRenderer() {
  glDeleteBuffers(1, &full_screen_quad_.vbo);
  glDeleteVertexArrays(1, &full_screen_quad_.vao);
  glDeleteTextures(1, &white_texture_);
  glDeleteTextures(1, &black_texture_);

  auto mat_sys = static_cast<GlMaterialSystem *>(engine_->GetMaterial());
  mat_sys->ForceFreeFramebuffer(hdr_buffer_);
  mat_sys->ForceFreeFramebuffer(frame_buffer_);
}

void GlDeferredRenderer::RenderFrame(float dt) {
  auto frame_time = cu::TimerStart();
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Render Frame",
                  __FILE__, __LINE__);

  auto cull_system = engine_->GetCulling();
  auto mat_system = static_cast<GlMaterialSystem *>(engine_->GetMaterial());
  auto window = engine_->GetWindow();

  auto cam_entities = g_ent_mgr.GetEbt<Camera>();
  auto camera_comps = g_ent_mgr.GetOldCbt<Camera>();

  if (camera_comps) {
    for (int i = 0; i < camera_comps->size(); ++i) {
      auto opeque_mesh_packs = cull_system->GetMeshPacks(cam_entities->at(i));
      if (!opeque_mesh_packs || opeque_mesh_packs->empty()) continue;

      mat_system->PushFrameBuffer(frame_buffer_);
      glClearColor(.0f, .0f, .0f, .0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      auto &cam = camera_comps->at(i);

      cu::AssertError(glGetError() == GL_NO_ERROR,
                      "OpenGL error - Render Frame", __FILE__, __LINE__);
      deferred_shading_effect_->DrawGBuffers(cam, *opeque_mesh_packs);
      cu::AssertError(glGetError() == GL_NO_ERROR,
                      "OpenGL error - Render Frame", __FILE__, __LINE__);

      if (g_settings.Ssao()) ssao_effect_->ApplySsaoEffect(cam);
      cu::AssertError(glGetError() == GL_NO_ERROR,
                      "OpenGL error - Render Frame", __FILE__, __LINE__);

      // Draw ambient lighting
      mat_system->PushFrameBuffer(hdr_buffer_);
      glClearColor(.0f, .0f, .0f, .0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      mat_system->ApplyMaterial(deffered_ambient_material_);
      auto shader_program = mat_system->GetCurrentShader();
      if (shader_locs_[0] == -1)
        shader_locs_[0] = glGetUniformLocation(shader_program, "cam_pos");
      glUniform3fv(shader_locs_[0], 1, (float *)&cam.position_);

      cu::AssertError(glGetError() == GL_NO_ERROR,
                      "OpenGL error - Render Frame", __FILE__, __LINE__);
      if (!g_settings.Ssao()) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, white_texture_);
        if (shader_locs_[1] == -1)
          shader_locs_[1] = glGetUniformLocation(shader_program, "ssao_tex");
        glUniform1i(shader_locs_[1], 3);
      }

      glBindVertexArray(full_screen_quad_.vao);
      glDrawArrays(GL_TRIANGLES, 0, 3);

      cu::AssertError(glGetError() == GL_NO_ERROR,
                      "OpenGL error - Render Frame", __FILE__, __LINE__);
      mat_system->PushFrameBuffer(hdr_buffer_, frame_buffer_);
      cu::AssertError(glGetError() == GL_NO_ERROR,
                      "OpenGL error - Render Frame", __FILE__, __LINE__);
      auto dim = window->GetRenderDim();
      glBlitFramebuffer(0, 0, dim.first, dim.second, 0, 0, dim.first,
                        dim.second, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
      auto err = glGetError();
      cu::AssertError(err == GL_NO_ERROR, "OpenGL error - Render Frame",
                      __FILE__, __LINE__);
      mat_system->PopFrameBuffer();

      cu::AssertError(glGetError() == GL_NO_ERROR,
                      "OpenGL error - Render Frame", __FILE__, __LINE__);
      deferred_lighting_effect_->DrawLights(cam, cam_entities->at(i),
                                            (float *)&cam.position_);
      skybox_effect_->DrawSkybox(cam, cam_entities->at(i));
      cu::AssertError(glGetError() == GL_NO_ERROR,
                      "OpenGL error - Render Frame", __FILE__, __LINE__);

      auto particle_emitters = g_ent_mgr.GetEbt<ParticleEmitter>();
      if (particle_emitters && !particle_emitters->empty()) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        auto part_sys = engine_->ParticleSystem();
        for (auto &e : *particle_emitters)
          part_sys->DrawParticleEmitter(e, cam, depth_desc_);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
      }

      // Draw translucent meshes
      auto translucent_mesh_packs =
          cull_system->GetMeshPacks(cam_entities->at(i), false);
      if (translucent_mesh_packs && !translucent_mesh_packs->empty())
        deferred_shading_effect_->DrawTranslucents(cam,
                                                   *translucent_mesh_packs);

      // Extract and blur the bloom texture
      if (g_settings.Bloom()) bloom_effect_->ApplyBloomEffect();

      // Draw final output with tonemapping, gamma correction and bloom
      mat_system->PopFrameBuffer();
      mat_system->PopFrameBuffer();

      if (g_settings.Smaa()) {
        glBindFramebuffer(GL_FRAMEBUFFER, smaa_effect_->OutputFramebuffer());
        glViewport(0, 0, dim.first, dim.second);
        glClear(GL_COLOR_BUFFER_BIT);
      } else {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthMask(GL_FALSE);
      }

      mat_system->ApplyMaterial(tonemap_gamma_material_);
      shader_program = mat_system->GetCurrentShader();

      if (!g_settings.Bloom()) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, black_texture_);
        if (shader_locs_[3] == -1)
          shader_locs_[3] =
              glGetUniformLocation(shader_program, "bloom_buffer");
        glUniform1i(shader_locs_[3], 1);
      }

      if (shader_locs_[4] == -1)
        shader_locs_[4] = glGetUniformLocation(shader_program, "exposure");
      glUniform1f(shader_locs_[4], cam.exposure_);

      if (shader_locs_[5] == -1)
        shader_locs_[5] = glGetUniformLocation(shader_program, "gamma");
      glUniform1f(shader_locs_[5], g_settings.Gamma());

      if (shader_locs_[6] == -1)
        shader_locs_[6] = glGetUniformLocation(shader_program, "hue");
      glUniform1f(shader_locs_[6], g_settings.Hue());

      if (shader_locs_[7] == -1)
        shader_locs_[7] = glGetUniformLocation(shader_program, "saturation");
      glUniform1f(shader_locs_[7], g_settings.Saturation());

      if (shader_locs_[8] == -1)
        shader_locs_[8] = glGetUniformLocation(shader_program, "brightness");
      glUniform1f(shader_locs_[8], g_settings.Brightness());

      glBindVertexArray(full_screen_quad_.vao);
      glDrawArrays(GL_TRIANGLES, 0, 3);

      if (g_settings.Smaa()) smaa_effect_->ApplySmaa();

      cu::AssertError(glGetError() == GL_NO_ERROR,
                      "OpenGL error - Render Frame", __FILE__, __LINE__);

      auto win_dim = window->GetWindowDim();
      gui_renderer_->Draw({float(win_dim.first), float(win_dim.second)});
      cu::AssertError(glGetError() == GL_NO_ERROR,
                      "OpenGL error - Render Frame", __FILE__, __LINE__);

      glDepthMask(GL_TRUE);
    }
  }

  engine_->GetDebugOutput()->UpdateBottomRightLine(
      0,
      std::to_string(cu::TimerStop<std::milli>(frame_time)) + " :Render time");
}

void GlDeferredRenderer::InitRenderer() {
  auto mat_system = static_cast<GlMaterialSystem *>(engine_->GetMaterial());
  auto window = engine_->GetWindow();
  auto gui_factory = lib_gui::GuiFactory();

  QuadVert quad_vertices[] = {{{-1.0f, 3.0f}, {0.0f, 2.f}},
                              {{-1.0f, -1.0f}, {0.0f, 0.0f}},
                              {{3.0f, -1.0f}, {2.f, 0.0f}}};

  gui_renderer_ = gui_factory.CreateGuiRenderer(engine_);

  if (!window->CheckCapabilities()) return;
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Render Frame",
                  __FILE__, __LINE__);

  GLint uniform_vert_limit, uniform_frag_limit;
  glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &uniform_vert_limit);
  glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &uniform_frag_limit);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  auto dim = window->GetRenderDim();
  auto deferred_fb_command =
      CreateDeferedFrameBufferCommand(dim.first, dim.second);
  issue_command(deferred_fb_command);
  frame_buffer_ = deferred_fb_command.FrameBufferId();

  auto hdr_fb_command = CreateHdrFrameBufferCommand(dim.first, dim.second);
  issue_command(hdr_fb_command);
  hdr_buffer_ = hdr_fb_command.FrameBufferId();

  glViewport(0, 0, dim.first, dim.second);

  glGenVertexArrays(1, &full_screen_quad_.vao);
  glGenBuffers(1, &full_screen_quad_.vbo);

  glBindVertexArray(full_screen_quad_.vao);
  glBindBuffer(GL_ARRAY_BUFFER, full_screen_quad_.vbo);

  glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(QuadVert), &quad_vertices[0],
               GL_STATIC_DRAW);

  // Vertex Positions
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVert),
                        (GLvoid *)0);

  // Vertex Texcoord
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVert),
                        (GLvoid *)offsetof(QuadVert, tex_coord));

  glBindVertexArray(0);

  ct::dyn_array<lib_core::Vector3> white_tex;
  for (GLuint i = 0; i < 4; ++i) white_tex.push_back({1.f, 1.f, 1.f});
  glGenTextures(1, &white_texture_);
  glBindTexture(GL_TEXTURE_2D, white_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_FLOAT,
               &white_tex[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  ct::dyn_array<lib_core::Vector3> black_tex;
  for (GLuint i = 0; i < 4; ++i) black_tex.push_back({.0f, .0f, .0f});
  glGenTextures(1, &black_texture_);
  glBindTexture(GL_TEXTURE_2D, black_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_FLOAT,
               &black_tex[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  blur_effect_ = std::make_unique<GlGausianBlur>(engine_);
  blur_effect_->SetScreenQuad(full_screen_quad_.vao);

  smaa_effect_ = std::make_unique<GlSmaa>(engine_);

  bloom_effect_ = std::make_unique<GlBloom>(
      std::pair<size_t, size_t>(dim.first / 2, dim.second / 2), engine_,
      blur_effect_.get(),
      TextureDesc({deferred_fb_command.RmeTextureId(), "g_rma"}),
      TextureDesc({hdr_fb_command.HdrTextureId(), "hdr_buffer"}));
  bloom_effect_->SetScreenQuad(full_screen_quad_.vao);

  ssao_effect_ = std::make_unique<GlSsao>(
      std::pair<size_t, size_t>(dim.first / 2, dim.second / 2),
      blur_effect_.get(), engine_,
      TextureDesc({deferred_fb_command.PositionTextureId(), "g_position"}),
      TextureDesc({deferred_fb_command.NormalTextureId(), "g_normal"}));
  ssao_effect_->SetScreenQuad(full_screen_quad_.vao);

  deferred_shading_effect_ = std::make_unique<GlDeferredShading>(engine_);

  skybox_effect_ = std::make_unique<GlSkyboxShading>(engine_);

  deferred_lighting_effect_ = std::make_unique<GlDeferredLighting>(
      engine_,
      TextureDesc({deferred_fb_command.PositionTextureId(), "g_position"}),
      TextureDesc({deferred_fb_command.NormalTextureId(), "g_normal"}),
      TextureDesc({deferred_fb_command.AlbedoTextureId(), "g_albedo"}),
      TextureDesc({deferred_fb_command.RmeTextureId(), "g_rma"}),
      TextureDesc({deferred_fb_command.DepthTextureId(), "g_depth"}));
  deferred_lighting_effect_->SetScreenQuad(full_screen_quad_.vao);

  Material material;
  material.textures.push_back(
      TextureDesc({deferred_fb_command.AlbedoTextureId(), "g_albedo"}));
  material.textures.push_back(
      TextureDesc({deferred_fb_command.RmeTextureId(), "g_rma"}));
  material.textures.push_back(
      {deferred_fb_command.DepthTextureId(), "g_depth"});
  material.textures.push_back(ssao_effect_->GetSsaoTexture());
  material.textures.back().name = "ssao_tex";

  material.shader = mat_system->GetStockShaderId(
      MaterialSystem::ShaderType::kDeferredLightingAmbient);

  auto material_command = AddMaterialCommand(material);
  issue_command(material_command);
  deffered_ambient_material_ = material_command.MaterialId();

  material.textures.clear();
  material.shader =
      mat_system->GetStockShaderId(MaterialSystem::ShaderType::kTonemapGamma);
  material.textures.push_back(
      TextureDesc({hdr_fb_command.HdrTextureId(), "hdr_buffer"}));
  material.textures.push_back(bloom_effect_->GetBloomTexture());

  depth_desc_ = {deferred_fb_command.DepthTextureId(), "g_depth"};

  material_command = AddMaterialCommand(material);
  issue_command(material_command);
  tonemap_gamma_material_ = material_command.MaterialId();

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Render Frame",
                  __FILE__, __LINE__);
}

void GlDeferredRenderer::Clear(lib_core::Vector4 color) {
  glClearColor(color[0], color[1], color[2], color[3]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
}  // namespace lib_graphics
