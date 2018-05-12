#ifdef WindowsBuild
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WIN32_WINDOWS 0x0601
#define NOMINMAX
#include <windows.h>
#endif

#include <GL/glew.h>

#ifdef UnixBuild
#include <GL/gl.h>
#elif WindowsBuild
#include <GL/GL.h>
#endif

#include "entity_manager.h"
#include "gl_rect_system.h"
#include "gui_rect.h"

namespace lib_gui {
GlRectSystem::GlRectSystem() {}

GlRectSystem::~GlRectSystem() {}

void GlRectSystem::DrawUpdate(lib_graphics::Renderer *renderer,
                              lib_gui::TextSystem *text_renderer) {
  if (!init_draw_) {
    CreateRectResources();
    init_draw_ = true;
  }
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Draw Rectangle",
                  __FILE__, __LINE__);
}

void GlRectSystem::DrawRect(GuiRect &rect, lib_core::Vector2 screen_dim) {
  lib_core::Matrix4x4 projection;
  projection.Orthographic(0.0f, screen_dim[0], 0.0f, screen_dim[1]);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBindVertexArray(rect_vao_);
  glUseProgram(shader_);
  glUniform4fv(color_loc_, 1, &rect.rgba_[0]);
  glUniformMatrix4fv(proj_loc_, 1, GL_FALSE, projection.data);

  auto a_ratio = screen_dim[0] / screen_dim[1];

  auto x_pos = rect.position_[0] * screen_dim[0];
  auto x_half = rect.half_size_[0] * screen_dim[0];
  auto y_pos = rect.position_[1] * screen_dim[1];
  auto y_half = rect.half_size_[1] * screen_dim[1] * a_ratio;
  GLfloat vertices[6][2] = {
      {x_pos - x_half, y_pos + y_half}, {x_pos - x_half, y_pos - y_half},
      {x_pos + x_half, y_pos - y_half}, {x_pos - x_half, y_pos + y_half},
      {x_pos + x_half, y_pos - y_half}, {x_pos + x_half, y_pos + y_half}};

  glBindBuffer(GL_ARRAY_BUFFER, rect_vbo_);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  glBindVertexArray(0);
  glDisable(GL_BLEND);
}

void GlRectSystem::PurgeGpuResources() {
  glDeleteVertexArrays(1, &rect_vao_);
  glDeleteBuffers(1, &rect_vbo_);
  glDeleteProgram(shader_);
  init_draw_ = false;
}

void GlRectSystem::CreateRectResources() {
  ct::string vert_shader =
      "#version 330 core\n"
      "layout(location = 0) in vec2 vertex;\n"

      "uniform mat4 projection;\n\n"

      "void main(){\n"
      "  gl_Position = projection * vec4(vertex, 0.0, 1.0);\n"
      "}";

  ct::string frag_shader =
      "#version 330 core\n"
      "out vec4 color;\n\n"

      "uniform vec4 rect_color;\n\n"

      "void main() {\n"
      "  color = rect_color;\n"
      "}";

  glGenVertexArrays(1, &rect_vao_);
  glGenBuffers(1, &rect_vbo_);
  glBindVertexArray(rect_vao_);
  glBindBuffer(GL_ARRAY_BUFFER, rect_vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 2, NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  GLuint vertex_shader;
  vertex_shader = glCreateShader(GL_VERTEX_SHADER);

  const char *code_char = vert_shader.c_str();
  glShaderSource(vertex_shader, 1, &code_char, NULL);
  glCompileShader(vertex_shader);

  GLint success;
  char info_log[512];
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);

  if (!success) {
    glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);

    ct::string error = "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n";
    error += info_log;
    cu::AssertError(success > 0, error, __FILE__, __LINE__);
  }

  GLuint fragment_shader;
  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

  code_char = frag_shader.c_str();
  glShaderSource(fragment_shader, 1, &code_char, NULL);
  glCompileShader(fragment_shader);

  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);

  if (!success) {
    glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);

    ct::string error = "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n";
    error += info_log;
    cu::AssertError(success > 0, error, __FILE__, __LINE__);
  }

  shader_ = glCreateProgram();
  glAttachShader(shader_, vertex_shader);
  glAttachShader(shader_, fragment_shader);
  glLinkProgram(shader_);

  glGetProgramiv(shader_, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shader_, 512, NULL, info_log);

    ct::string error_str = "ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n";
    error_str += info_log;
    cu::AssertError(success > 0, error_str, __FILE__, __LINE__);
  }

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  color_loc_ = glGetUniformLocation(shader_, "rect_color");
  proj_loc_ = glGetUniformLocation(shader_, "projection");
}
}  // namespace lib_gui
