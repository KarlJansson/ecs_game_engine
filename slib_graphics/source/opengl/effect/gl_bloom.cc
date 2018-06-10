#include "gl_bloom.h"
#include <GL/glew.h>

#include <utility>
#include "gl_gausian_blur.h"
#include "gl_material_system.h"
#include "gl_window.h"

namespace lib_graphics {
GlBloom::GlBloom(std::pair<size_t, size_t> dim, lib_core::EngineCore *engine,
                 GlGausianBlur *blur_effect, const TextureDesc &rme_gbuffer,
                 const TextureDesc &hdr_buffer)
    : bloom_dim_(std::move(dim)), engine_(engine), blur_effect_(blur_effect) {
  ct::string vert_shader =
      "#version 430 core\n"
      "layout(location = 0) in vec2 position;\n"
      "layout(location = 1) in vec2 texCoords;\n"

      "out vec2 TexCoords;\n"

      "void main()\n"
      "{\n"
      "  gl_Position = vec4(position.x, position.y, 0.0f, 1.0f);\n"
      "  TexCoords = texCoords;\n"
      "}";

  ct::string frag_shader =
      "#version 430 core\n"
      "in vec2 TexCoords;\n"
      "out vec4 color;\n"

      "uniform sampler2D hdr_buffer;\n"
      "uniform sampler2D g_rma;\n"

      "void main()\n"
      "{\n"
      "  float emissive_color = texture(g_rma, TexCoords).b;\n"
      "  vec4 hdr_color = texture(hdr_buffer, TexCoords);\n"
      "  float brightness = dot(hdr_color.rgb, vec3(0.2126, 0.7152, 0.0722));\n"
      "  if (brightness > 1.0 || emissive_color > 0.5)\n"
      "    color = vec4(vec3(emissive_color) - exp(-hdr_color.rgb), .5);\n"
      "  else discard;\n"
      "}";

  Material material;
  auto shader_command = AddShaderCommand(vert_shader, frag_shader);
  issue_command(shader_command);
  bloom_shader_ = shader_command.ShaderId();

  material.shader = shader_command.ShaderId();
  material.textures.push_back(hdr_buffer);
  material.textures.push_back(rme_gbuffer);

  auto material_command = AddMaterialCommand(material);
  issue_command(material_command);
  bloom_extract_material_ = material_command.MaterialId();

  auto bloom_fbs = CreateBlurFrameBufferCommand(
      bloom_dim_.first, bloom_dim_.second, GL_RGB, false);
  issue_command(bloom_fbs);

  bloom_buffer_.push_back(bloom_fbs.FrameBuffer1Id());
  bloom_buffer_.push_back(bloom_fbs.FrameBuffer2Id());

  bloom_return_desc_ = {bloom_fbs.Texture1Id(), "bloom_buffer"};
}

GlBloom::~GlBloom() {
  issue_command(RemoveShaderCommand(bloom_shader_));
  issue_command(RemoveMaterialCommand(bloom_extract_material_));

  auto gl_mat_sys = static_cast<GlMaterialSystem *>(engine_->GetMaterial());
  gl_mat_sys->ForceFreeFramebuffer(bloom_buffer_[0]);
  gl_mat_sys->ForceFreeFramebuffer(bloom_buffer_[1]);
}

void GlBloom::ApplyBloomEffect() {
  auto bloom_timer = cu::TimerStart();
  auto mat_system = engine_->GetMaterial();

  mat_system->PushFrameBuffer(bloom_buffer_[0]);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  mat_system->ApplyMaterial(bloom_extract_material_);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBindVertexArray(screen_quad_);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glDisable(GL_BLEND);

  blur_effect_->ApplyBlurEffect(bloom_buffer_, 2);

  mat_system->PopFrameBuffer();
  engine_->GetDebugOutput()->UpdateBottomRightLine(
      5,
      std::to_string(cu::TimerStop<std::milli>(bloom_timer)) + " :Bloom time");
}

void GlBloom::SetBloomTextureSize(std::pair<size_t, size_t> dim) {}

void GlBloom::SetScreenQuad(unsigned quad) { screen_quad_ = quad; }

TextureDesc &GlBloom::GetBloomTexture() { return bloom_return_desc_; }
}  // namespace lib_graphics
