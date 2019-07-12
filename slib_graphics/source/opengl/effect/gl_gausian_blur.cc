#include "gl_gausian_blur.h"
#include <GL/glew.h>
#include "material_system.h"

namespace lib_graphics {
GlGausianBlur::GlGausianBlur(lib_core::EngineCore *engine) : engine_(engine) {
  auto shader_command = AddShaderCommand(
      cu::ReadFile("./content/shaders/opengl/gaussian_blur_vs.glsl"),
      cu::ReadFile("./content/shaders/opengl/gaussian_blur_fs.glsl"));
  blur_material_.shader = shader_command.ShaderId();
  issue_command(shader_command);

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
