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

#include <ft2build.h>
#include <iostream>
#include "gl_text_system.h"
#include "system_manager.h"
#include FT_FREETYPE_H

namespace lib_gui {
void GlTextSystem::RenderText(GuiText text, lib_core::Vector2 screen_dim) {
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Draw Text.",
                  __FILE__, __LINE__);

  auto font_loc = font_id_mapping_.find(text.font);
  if (font_loc == font_id_mapping_.end()) return;

  auto char_set = fonts_.find(font_loc->second);
  if (char_set == fonts_.end()) return;

  lib_core::Matrix4x4 projection;
  projection.Orthographic(0.0f, screen_dim[0], 0.0f, screen_dim[1]);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(text_vao);
  glUseProgram(shader_);

  glUniform3fv(color_loc_, 1, &text.rgba[0]);
  glUniformMatrix4fv(proj_loc_, 1, GL_FALSE, projection.data);

  Character ch;
  GLfloat xpos, ypos, w, h;
  auto tmp_x = text.position[0] * screen_dim[0];
  auto tmp_y = text.position[1] * screen_dim[1];
  float max_x = 0.f;

  for (auto c = text.text.begin(); c != text.text.end(); c++) {
    ch = char_set->second.glyphs[*c];
    max_x = tmp_x + ch.bearing.first * text.half_size[0];
    tmp_x += (ch.advance >> 6) * text.half_size[0];
  }

  switch (text.h_alignment) {
    case GuiText::kLeft:
      tmp_x = text.position[0] * screen_dim[0];
      break;
    case GuiText::kRight:
      tmp_x = text.position[0] * screen_dim[0] -
              (max_x - text.position[0] * screen_dim[0]);
      break;
    case GuiText::kCenter:
      tmp_x = text.position[0] * screen_dim[0] -
              (max_x - text.position[0] * screen_dim[0]) * .5f;
      tmp_x -= char_set->second.font_size * .25f;
      tmp_y -= char_set->second.font_size * .25f;
      break;
  }

  for (auto c = text.text.begin(); c != text.text.end(); c++) {
    ch = char_set->second.glyphs[*c];

    xpos = tmp_x + ch.bearing.first * text.half_size[0];
    ypos = tmp_y - (ch.size.second - ch.bearing.second) * text.half_size[1];

    w = ch.size.first * text.half_size[0];
    h = ch.size.second * text.half_size[1];
    GLfloat vertices[6][4] = {
        {xpos, ypos + h, 0.0, 0.0}, {xpos, ypos, 0.0, 1.0},
        {xpos + w, ypos, 1.0, 1.0}, {xpos, ypos + h, 0.0, 0.0},
        {xpos + w, ypos, 1.0, 1.0}, {xpos + w, ypos + h, 1.0, 0.0}};
    glBindTexture(GL_TEXTURE_2D, ch.texture_id);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    tmp_x += (ch.advance >> 6) * text.half_size[0];
  }

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_BLEND);
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Draw text.",
                  __FILE__, __LINE__);
}

void GlTextSystem::PurgeGpuResources() {
  for (auto &f : fonts_)
    for (auto &c : f.second.glyphs) glDeleteTextures(1, &c.texture_id);
  fonts_.clear();
  shared_resource_lookup_.clear();

  glDeleteVertexArrays(1, &text_vao);
  glDeleteBuffers(1, &text_vbo);
  glDeleteProgram(shader_);
  color_loc_ = -1;
  init_draw_ = false;
}

void GlTextSystem::DrawUpdate(lib_graphics::Renderer *renderer,
                              lib_gui::TextSystem *text_renderer) {
  if (!init_draw_) {
    CreateTextResources();
    auto load_font_commands = g_sys_mgr.GetCommands<LoadFontCommand>();
    for (auto &command : loaded_fonts_)
      load_font_commands->push_back(command.second);

    init_draw_ = true;
  }

  TextSystem::DrawUpdate(renderer, text_renderer);
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error.", __FILE__,
                  __LINE__);
}

void GlTextSystem::HandleUnloadCommand(UnloadFontCommand &command) {
  auto font_loc = font_id_mapping_.find(command.font_id);
  if (font_loc == font_id_mapping_.end()) return;

  auto it = fonts_.find(font_loc->second);
  font_id_mapping_.erase(font_loc);
  if (it != fonts_.end()) {
    --it->second.ref_count;
    if (it->second.ref_count > 0) return;

    for (auto &c : it->second.glyphs) glDeleteTextures(1, &c.texture_id);
    shared_resource_lookup_.erase(it->second.hash);
    loaded_fonts_.erase(it->first);
    fonts_.erase(it);
  } else
    missing_removed_.insert(command.font_id);
}

void GlTextSystem::HandleLoadCommand(LoadFontCommand &command) {
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error.", __FILE__,
                  __LINE__);

  FT_Library ft;
  if (FT_Init_FreeType(&ft))
    std::cout << "ERROR::FREETYPE: Could not init FreeType Library"
              << std::endl;

  FT_Face face;
  if (FT_New_Face(ft, command.path.c_str(), 0, &face))
    std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;

  FT_Set_Pixel_Sizes(face, 0, command.size);
  if (FT_Load_Char(face, 'X', FT_LOAD_RENDER))
    std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;

  glPixelStorei(GL_UNPACK_ALIGNMENT,
                1);  // Disable byte-alignment restriction

  ct::dyn_array<Character> characters(128);
  for (GLubyte c = 0; c < 128; c++) {
    // Load character glyph
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
      std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
      continue;
    }
    // Generate texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width,
                 face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
                 face->glyph->bitmap.buffer);
    // Set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Now store character for later use
    Character character = {
        texture,
        {face->glyph->bitmap.width, face->glyph->bitmap.rows},
        {face->glyph->bitmap_left, face->glyph->bitmap_top},
        unsigned(face->glyph->advance.x)};
    characters[c] = std::move(character);
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  FT_Done_Face(face);
  FT_Done_FreeType(ft);

  fonts_[command.FontId()].font_size = command.size;
  fonts_[command.FontId()].glyphs.swap(characters);

  if (loaded_fonts_.find(command.FontId()) == loaded_fonts_.end())
    loaded_fonts_[command.FontId()] = command;

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error.", __FILE__,
                  __LINE__);
}

void GlTextSystem::CreateTextResources() {
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error.", __FILE__,
                  __LINE__);
  ct::string vert_shader =
      "#version 430 core\n"
      "layout(location = 0) in vec4 vertex;\n"
      "out vec2 TexCoords;\n\n"

      "uniform mat4 projection;\n\n"

      "void main(){\n"
      "  gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
      "  TexCoords = vertex.zw;\n"
      "}";

  ct::string frag_shader =
      "#version 430 core\n"
      "in vec2 TexCoords;\n"
      "out vec4 color;\n\n"

      "uniform sampler2D text;\n"
      "uniform vec3 textColor;\n\n"

      "void main() {\n"
      "  color = vec4(textColor, texture(text, TexCoords).r);\n"
      "}";

  glGenVertexArrays(1, &text_vao);
  glGenBuffers(1, &text_vbo);
  glBindVertexArray(text_vao);
  glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  GLuint vertex_shader;
  vertex_shader = glCreateShader(GL_VERTEX_SHADER);

  const char *code_char = vert_shader.c_str();
  glShaderSource(vertex_shader, 1, &code_char, NULL);
  glCompileShader(vertex_shader);

  GLint success;
  GLchar info_log[512];
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);

  if (!success) {
    glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);

    ct::string error_str = "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n";
    error_str += info_log;
    cu::AssertError(success > 0, error_str, __FILE__, __LINE__);
  }

  GLuint fragment_shader;
  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

  code_char = frag_shader.c_str();
  glShaderSource(fragment_shader, 1, &code_char, NULL);
  glCompileShader(fragment_shader);

  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);

  if (!success) {
    glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
    ct::string error_str = "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n";
    error_str += info_log;
    cu::AssertError(success > 0, error_str, __FILE__, __LINE__);
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

  proj_loc_ = glGetUniformLocation(shader_, "projection");
  color_loc_ = glGetUniformLocation(shader_, "textColor");
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error.", __FILE__,
                  __LINE__);
}
}  // namespace lib_gui
