#include "gl_ssao.h"
#include <GL/glew.h>
#include <random>
#include <utility>
#include "gl_gausian_blur.h"
#include "gl_material_system.h"
#include "gl_window.h"

namespace lib_graphics {
GlSsao::GlSsao(std::pair<size_t, size_t> dim, GlGausianBlur *blur,
               lib_core::EngineCore *engine, const TextureDesc &pos_gbuffer,
               const TextureDesc &normal_gbuffer)
    : ssao_dim_(std::move(dim)), blur_effect_(blur), engine_(engine) {
  auto shader_command =
      AddShaderCommand(cu::ReadFile("./content/shaders/opengl/ssao_vs.glsl"),
                       cu::ReadFile("./content/shaders/opengl/ssao_fs.glsl"));
  ssao_shader_ = shader_command.ShaderId();
  issue_command(shader_command);

  Material material;
  material.shader = ssao_shader_;
  material.textures.push_back(pos_gbuffer);
  material.textures.push_back(normal_gbuffer);

  auto material_command = AddMaterialCommand(material);
  ssao_material_ = material_command.MaterialId();
  issue_command(material_command);

  auto lerp = [](GLfloat a, GLfloat b, GLfloat f) -> GLfloat {
    return a + f * (b - a);
  };
  std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
  std::default_random_engine generator;

  lib_core::Vector3 sample;
  for (GLuint i = 0; i < 64; ++i) {
    sample[0] = randomFloats(generator) * 2.0f - 1.0f;
    sample[1] = randomFloats(generator) * 2.0f - 1.0f;
    sample[2] = randomFloats(generator);
    sample.Normalize();

    sample *= randomFloats(generator);
    GLfloat scale = GLfloat(i) / 64.0f;
    scale = lerp(0.1f, 1.0f, scale * scale);
    sample *= scale;
    ssao_kernel_.push_back(sample[0]);
    ssao_kernel_.push_back(sample[1]);
    ssao_kernel_.push_back(sample[2]);
  }

  ct::dyn_array<lib_core::Vector3> ssaoNoise;
  for (GLuint i = 0; i < 16; i++) {
    ssaoNoise.push_back({});
    ssaoNoise.back()[0] = randomFloats(generator);
    ssaoNoise.back()[1] = randomFloats(generator);
    ssaoNoise.back()[2] = 0.0f;
  }

  glGenTextures(1, &noise_texture_);
  glBindTexture(GL_TEXTURE_2D, noise_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 4, 4, 0, GL_RGB, GL_FLOAT,
               &ssaoNoise[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  auto blur_fbs = CreateBlurFrameBufferCommand(ssao_dim_.first,
                                               ssao_dim_.second, GL_RED, false);
  issue_command(blur_fbs);
  ssao_buffer_.push_back(blur_fbs.FrameBuffer1Id());
  ssao_buffer_.push_back(blur_fbs.FrameBuffer2Id());

  ssao_return_tex_ = {blur_fbs.Texture1Id(), "ssao_tex"};
}

GlSsao::~GlSsao() {
  auto mat_system = static_cast<GlMaterialSystem *>(engine_->GetMaterial());
  mat_system->ForceFreeFramebuffer(ssao_buffer_[0]);
  mat_system->ForceFreeFramebuffer(ssao_buffer_[1]);

  issue_command(RemoveShaderCommand(ssao_shader_));
  issue_command(RemoveMaterialCommand(ssao_material_));
}

void GlSsao::ApplySsaoEffect(Camera &cam) {
  auto ssao_timer = cu::TimerStart();
  auto mat_system = engine_->GetMaterial();

  mat_system->PushFrameBuffer(ssao_buffer_[0]);
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  mat_system->ApplyMaterial(ssao_material_);
  CheckShaderLocations();

  std::array<float, 2> ssao_dim = {ssao_dim_.first / 4.0f,
                                   ssao_dim_.second / 4.0f};

  glUniformMatrix4fv(view_loc_, 1, GL_FALSE, cam.view_.data);
  glUniformMatrix4fv(proj_loc_, 1, GL_FALSE, cam.proj_.data);
  glUniform1i(ksize_loc_, kernel_smaples_);
  glUniform1f(radius_loc_, radius_);
  glUniform1f(bias_loc_, bias_);
  glUniform2fv(nscale_loc_, 1, ssao_dim.data());

  glUniform3fv(samples_loc_, kernel_smaples_, ssao_kernel_.data());

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, noise_texture_);

  glUniform1i(noise_loc_, 2);

  glBindVertexArray(screen_quad_);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  mat_system->PopFrameBuffer();

  blur_effect_->ApplyBlurEffect(ssao_buffer_, 1);
  engine_->GetDebugOutput()->UpdateBottomRightLine(
      4, std::to_string(cu::TimerStop<std::milli>(ssao_timer)) + " :Ssao time");
}

void GlSsao::SetScreenQuad(unsigned quad) { screen_quad_ = quad; }

TextureDesc &GlSsao::GetSsaoTexture() { return ssao_return_tex_; }

void GlSsao::CheckShaderLocations() {
  auto mat_system = static_cast<GlMaterialSystem *>(engine_->GetMaterial());
  auto shader_program = mat_system->GetCurrentShader();
  if (view_loc_ == -1)
    view_loc_ = glGetUniformLocation(shader_program, "view");
  else
    return;

  if (proj_loc_ == -1)
    proj_loc_ = glGetUniformLocation(shader_program, "projection");
  if (ksize_loc_ == -1)
    ksize_loc_ = glGetUniformLocation(shader_program, "kernelSize");
  if (radius_loc_ == -1)
    radius_loc_ = glGetUniformLocation(shader_program, "radius");
  if (nscale_loc_ == -1)
    nscale_loc_ = glGetUniformLocation(shader_program, "noise_scale");
  if (bias_loc_ == -1) bias_loc_ = glGetUniformLocation(shader_program, "bias");
  if (samples_loc_ == -1)
    samples_loc_ = glGetUniformLocation(shader_program, "samples[0]");
  if (noise_loc_ == -1)
    noise_loc_ = glGetUniformLocation(shader_program, "tex_noise");
}
}  // namespace lib_graphics
