#include "gl_gausian_blur.h"
#include <GL/glew.h>
#include "material_system.h"

namespace lib_graphics {
GlGausianBlur::GlGausianBlur(lib_core::EngineCore *engine) : engine_(engine) {
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
      "out vec4 FragColor;\n"
      "in vec2 TexCoords;\n"

      "uniform sampler2D blur_image;\n"

      "uniform int horizontal;\n"

      "vec4 blur13(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) "
      "{\n"
      "  vec4 color = vec4(0.0);\n"
      "  vec2 off1 = vec2(1.411764705882353) * direction;\n"
      "  vec2 off2 = vec2(3.2941176470588234) * direction;\n"
      "  vec2 off3 = vec2(5.176470588235294) * direction;\n"
      "  color += texture(image, uv) * 0.1964825501511404;\n"
      "  color += texture(image, uv + (off1 / resolution)) * "
      "0.2969069646728344;\n"
      "  color += texture(image, uv - (off1 / resolution)) * "
      "0.2969069646728344;\n"
      "  color += texture(image, uv + (off2 / resolution)) * "
      "0.09447039785044732;\n"
      "  color += texture(image, uv - (off2 / resolution)) * "
      "0.09447039785044732;\n"
      "  color += texture(image, uv + (off3 / resolution)) * "
      "0.010381362401148057;\n"
      "  color += texture(image, uv - (off3 / resolution)) * "
      "0.010381362401148057;\n"
      "  return color;\n"
      "}\n"

      "vec4 blur9(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) "
      "{\n"
      "  vec4 color = vec4(0.0);\n"
      "  vec2 off1 = vec2(1.3846153846) * direction;\n"
      "  vec2 off2 = vec2(3.2307692308) * direction;\n"
      "  color += texture2D(image, uv) * 0.2270270270;\n"
      "  color += texture2D(image, uv + (off1 / resolution)) * 0.3162162162;\n"
      "  color += texture2D(image, uv - (off1 / resolution)) * 0.3162162162;\n"
      "  color += texture2D(image, uv + (off2 / resolution)) * 0.0702702703;\n"
      "  color += texture2D(image, uv - (off2 / resolution)) * 0.0702702703;\n"
      "  return color;\n"
      "}\n"

      "vec4 blur5(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) "
      "{\n"
      "  vec4 color = vec4(0.0);\n"
      "  vec2 off1 = vec2(1.3333333333333333) * direction;\n"
      "  color += texture2D(image, uv) * 0.29411764705882354;\n"
      "  color += texture2D(image, uv + (off1 / resolution)) * "
      "0.35294117647058826;\n"
      "  color += texture2D(image, uv - (off1 / resolution)) * "
      "0.35294117647058826;\n"
      "  return color;\n"
      "}\n"

      "void main()\n"
      "{\n"
      "  vec2 tex_resolution = textureSize(blur_image, 0);\n"
      "  vec2 direction = vec2(horizontal, 1 - horizontal);"
      "  FragColor = blur13(blur_image, TexCoords, tex_resolution, "
      "direction);\n"
      "}";

  auto shader_command = AddShaderCommand(vert_shader, frag_shader);
  issue_command(shader_command);
  blur_material_.shader = shader_command.ShaderId();
  blur_material_.textures.push_back({0, "blur_image"});
}

GlGausianBlur::~GlGausianBlur() {
  issue_command(RemoveShaderCommand(blur_material_.shader));
}

void GlGausianBlur::ApplyBlurEffect(ct::dyn_array<size_t> &blur_buffer,
                                    int passes) {
  auto mat_system = engine_->GetMaterial();

  int horizontal = 1;
  if (buffer_id_loc_ == -1) {
    mat_system->ForceMaterial(blur_material_);
    auto shader_program = mat_system->GetCurrentShader();
    buffer_id_loc_ = glGetUniformLocation(shader_program, "horizontal");
  }

  for (int i = 0; i < passes * 2; i++) {
    mat_system->PushFrameBuffer(blur_buffer[horizontal]);
    auto frame_buffer = mat_system->GetFrameBuffer(blur_buffer[horizontal]);

    blur_material_.textures[0].id = frame_buffer->textures[0].id;
    mat_system->ForceMaterial(blur_material_);

    glUniform1i(buffer_id_loc_, horizontal);
    glBindVertexArray(screen_quad_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    horizontal = horizontal == 0 ? 1 : 0;

    mat_system->PopFrameBuffer();
  }
}

void GlGausianBlur::SetScreenQuad(unsigned screen_quad) {
  screen_quad_ = screen_quad;
}
}  // namespace lib_graphics
