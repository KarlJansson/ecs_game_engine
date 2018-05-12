#include "gl_ssao.h"
#include <GL/glew.h>
#include <random>
#include "gl_gausian_blur.h"
#include "gl_material_system.h"
#include "gl_window.h"

namespace lib_graphics {
GlSsao::GlSsao(std::pair<size_t, size_t> dim, GlGausianBlur *blur,
               lib_core::EngineCore *engine, TextureDesc pos_gbuffer,
               TextureDesc normal_gbuffer)
    : ssao_dim_(dim), blur_effect_(blur), engine_(engine) {
  ct::string vert_shader =
      "#version 330 core\n"
      "layout(location = 0) in vec2 position;\n"
      "layout(location = 1) in vec2 texCoords;\n"

      "out vec2 TexCoords;\n"

      "void main()\n"
      "{\n"
      "  gl_Position = vec4(position.x, position.y, 0.0f, 1.0f);\n"
      "  TexCoords = texCoords;\n"
      "}";

  ct::string frag_shader =
      "#version 330 core\n"
      "out float FragColor;\n"
      "in vec2 TexCoords;\n"

      "uniform sampler2D g_position;\n"
      "uniform sampler2D g_normal;\n"
      "uniform sampler2D tex_noise;\n"

      "uniform vec3 samples[64];\n"
      "uniform mat4 projection;\n"
      "uniform mat4 view;\n"

      "uniform int kernelSize;\n"
      "uniform float radius;\n"
      "uniform float bias;\n"
      "uniform vec2 noise_scale;\n"

      "void main()\n"
      "{\n"
      "  vec3 fragPos = (view * vec4(texture(g_position, TexCoords).xyz, "
      "1)).xyz;\n"
      "  vec3 normal = (view * vec4(texture(g_normal, TexCoords).rgb * 2 - "
      "1.0, 0)).xyz;\n"
      "  normal = normalize(normal);\n"
      "  vec3 randomVec = texture(tex_noise, TexCoords * noise_scale).xyz * 2 "
      "- 1.0;\n"
      "  vec3 tangent = normalize(randomVec - normal * dot(randomVec, "
      "normal));\n"
      "  vec3 bitangent = cross(normal, tangent);\n"
      "  mat3 TBN = mat3(tangent, bitangent, normal);\n"
      "  float occlusion = 0.0;\n"
      "  for (int i = 0; i < kernelSize; ++i)\n"
      "  {\n"
      "    vec3 sample = TBN * samples[i];\n"
      "    sample = fragPos + sample * radius;\n"

      "    vec4 offset = vec4(sample, 1.0);\n"
      "    offset = projection * offset;\n"
      "    offset.xyz /= offset.w;\n"
      "    offset.xyz = offset.xyz * 0.5 + 0.5;\n"
      "    float sampleDepth = (view * vec4(texture(g_position, "
      "offset.xy))).z;\n"

      "    float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - "
      "sampleDepth));\n"
      "    occlusion += (sampleDepth >= sample.z + bias ? 1.0 : 0.0) * "
      "rangeCheck;\n"
      "  }\n"
      "  occlusion = 1.0 - (occlusion / kernelSize);\n"
      "  FragColor = occlusion;\n"
      "}";

  auto shader_command = AddShaderCommand(vert_shader, frag_shader);
  issue_command(shader_command);
  ssao_shader_ = shader_command.ShaderId();

  Material material;
  material.shader = shader_command.ShaderId();
  material.textures.push_back(pos_gbuffer);
  material.textures.push_back(normal_gbuffer);

  auto material_command = AddMaterialCommand(material);
  issue_command(material_command);
  ssao_material_ = material_command.MaterialId();

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

  float ssao_dim[] = {ssao_dim_.first / 4.0f, ssao_dim_.second / 4.0f};

  glUniformMatrix4fv(view_loc_, 1, GL_FALSE, cam.view_.data);
  glUniformMatrix4fv(proj_loc_, 1, GL_FALSE, cam.proj_.data);
  glUniform1i(ksize_loc_, kernel_smaples_);
  glUniform1f(radius_loc_, radius_);
  glUniform1f(bias_loc_, bias_);
  glUniform2fv(nscale_loc_, 1, ssao_dim);

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
