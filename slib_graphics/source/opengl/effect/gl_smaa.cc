#include "gl_smaa.h"
#include <GL/glew.h>
#include <fstream>
#include "gl_smaa_shaders.h"
#include "graphics_commands.h"
#include "window.h"

namespace lib_graphics {
GlSmaa::GlSmaa(lib_core::EngineCore *engine) : engine_(engine) {
  auto render_dim = engine_->GetWindow()->GetRenderDim();

  glGenTextures(1, &edge_tex_);
  glBindTexture(GL_TEXTURE_2D, edge_tex_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, render_dim.first, render_dim.second,
               0, GL_RGBA, GL_FLOAT, nullptr);

  glGenTextures(1, &blend_tex_);
  glBindTexture(GL_TEXTURE_2D, blend_tex_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, render_dim.first, render_dim.second,
               0, GL_RGBA, GL_FLOAT, nullptr);

  glGenTextures(1, &albedo_tex_);
  glBindTexture(GL_TEXTURE_2D, albedo_tex_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, render_dim.first, render_dim.second,
               0, GL_RGBA, GL_FLOAT, nullptr);

  std::ifstream smaa_area("./content/smaa_area.raw", std::ios::binary);
  cu::AssertError(!smaa_area.fail(), "Opening smaa area texture failed.",
                  __FILE__, __LINE__);

  ct::dyn_array<char> buffer;
  buffer.assign(160 * 560 * 2, 0);
  smaa_area.read(buffer.data(), buffer.size());
  smaa_area.close();

  glGenTextures(1, &area_tex_);
  glBindTexture(GL_TEXTURE_2D, area_tex_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, (GLsizei)160, (GLsizei)560, 0, GL_RG,
               GL_UNSIGNED_BYTE, buffer.data());

  std::ifstream smaa_search("./content/smaa_search.raw", std::ios::binary);
  cu::AssertError(!smaa_search.fail(), "Opening smaa search texture failed.",
                  __FILE__, __LINE__);

  buffer.clear();
  buffer.assign(66 * 33, 0);
  smaa_search.read(buffer.data(), buffer.size());
  smaa_search.close();

  glGenTextures(1, &search_tex_);
  glBindTexture(GL_TEXTURE_2D, search_tex_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, (GLsizei)66, (GLsizei)33, 0, GL_RED,
               GL_UNSIGNED_BYTE, buffer.data());

  GLenum modes[] = {GL_COLOR_ATTACHMENT0};
  glGenFramebuffers(1, &edge_fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, edge_fbo_);
  glDrawBuffers(1, modes);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         edge_tex_, 0);

  glGenFramebuffers(1, &blend_fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, blend_fbo_);
  glDrawBuffers(1, modes);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         blend_tex_, 0);

  glGenFramebuffers(1, &output_fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, output_fbo_);
  glDrawBuffers(1, modes);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         albedo_tex_, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  ct::string header =
      "#version 410 compatibility\n\
 #ifndef SMAA_PIXEL_SIZE\n\
 #define SMAA_PIXEL_SIZE vec2(" +
      std::to_string(1.f / float(render_dim.first)) + ", " +
      std::to_string(1.f / float(render_dim.second)) +
      ")\n\
 #endif\n\
";

  CreateShader((header + edge_vs), (header + edge_ps), &edge_shader_);
  glUseProgram(edge_shader_);
  glUniform1i(glGetUniformLocation(edge_shader_, "albedo_tex"), 0);
  glUseProgram(0);

  CreateShader((header + blend_vs), (header + blend_ps), &blend_shader_);
  glUseProgram(blend_shader_);
  glUniform1i(glGetUniformLocation(blend_shader_, "edge_tex"), 0);
  glUniform1i(glGetUniformLocation(blend_shader_, "area_tex"), 1);
  glUniform1i(glGetUniformLocation(blend_shader_, "search_tex"), 2);
  glUseProgram(0);

  CreateShader((header + neighborhood_vs), (header + neighborhood_ps),
               &neighborhood_shader_);
  glUseProgram(neighborhood_shader_);
  glUniform1i(glGetUniformLocation(neighborhood_shader_, "albedo_tex"), 0);
  glUniform1i(glGetUniformLocation(neighborhood_shader_, "blend_tex"), 1);
  glUseProgram(0);
}

GlSmaa::~GlSmaa() {
  GLuint textures[] = {edge_tex_, blend_tex_, area_tex_, search_tex_,
                       albedo_tex_};
  glDeleteTextures(5, &textures[0]);

  GLuint fbs[] = {edge_fbo_, blend_fbo_, output_fbo_};
  glDeleteFramebuffers(3, &fbs[0]);

  glDeleteProgram(edge_shader_);
  glDeleteProgram(blend_shader_);
  glDeleteProgram(neighborhood_shader_);
}

void GlSmaa::ApplySmaa() {
  auto smaa_timer = cu::TimerStart();

  glBindFramebuffer(GL_FRAMEBUFFER, edge_fbo_);

  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(edge_shader_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, albedo_tex_);

  glDrawArrays(GL_TRIANGLES, 0, 3);
  glUseProgram(0);

  // BLENDING WEIGHTS PASS
  glBindFramebuffer(GL_FRAMEBUFFER, blend_fbo_);

  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(blend_shader_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, edge_tex_);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, area_tex_);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, search_tex_);

  glDrawArrays(GL_TRIANGLES, 0, 3);
  glUseProgram(0);

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Smaa", __FILE__,
                  __LINE__);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  auto win_dim = engine_->GetWindow()->GetWindowDim();
  glViewport(0, 0, win_dim.first, win_dim.second);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDepthMask(GL_FALSE);

  // NEIGHBORHOOD BLENDING PASS
  glUseProgram(neighborhood_shader_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, albedo_tex_);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, blend_tex_);

  glDrawArrays(GL_TRIANGLES, 0, 3);
  glUseProgram(0);

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Smaa", __FILE__,
                  __LINE__);
  engine_->GetDebugOutput()->UpdateBottomRightLine(
      6, std::to_string(cu::TimerStop<std::milli>(smaa_timer)) + " :Smaa time");
}

unsigned GlSmaa::OutputFramebuffer() { return output_fbo_; }

void GlSmaa::ShaderInclude(std::string &shader) {
  size_t start_pos = 0;
  std::string include_dir = "#include ";

  while ((start_pos = shader.find(include_dir, start_pos)) !=
         std::string::npos) {
    auto pos = start_pos + include_dir.length() + 1;
    auto length = shader.find('\"', pos);
    std::string file = shader.substr(pos, length - pos);
    std::string content = "";

    std::ifstream f;
    f.open(("./content/" + file).c_str());

    if (f.is_open()) {
      char buffer[1024];

      while (!f.eof()) {
        f.getline(buffer, 1024);
        content += buffer;
        content += "\n";
      }
    } else {
      cu::AssertError(false, "smma shader file not found.", __FILE__, __LINE__);
    }

    shader.replace(start_pos, (length + 1) - start_pos, content);
    start_pos += content.length();
  }
}

void GlSmaa::ReplaceAll(std::string &str, const std::string &from,
                        const std::string &to) {
  size_t start_pos = 0;

  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();  // In case 'to' contains 'from', like replacing
                               // 'x' with 'yx'
  }
}

void GlSmaa::CreateShader(std::string vs_text, std::string ps_text,
                          unsigned *program) {
  GLuint shader;
  const GLchar *text_ptr[1];
  std::string log;
  GLint success;

  ShaderInclude(vs_text);
  ReplaceAll(vs_text, "hash ", "#");
  ShaderInclude(ps_text);
  ReplaceAll(ps_text, "hash ", "#");

  *program = glCreateProgram();

  // VERTEX SHADER
  text_ptr[0] = vs_text.c_str();
  GLchar info_log[512];

  shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(shader, 1, text_ptr, nullptr);
  glCompileShader(shader);

  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, 512, nullptr, info_log);

    ct::string error_str = "Vertex shader compilation failed:\n";
    error_str += info_log;
    cu::AssertError(success > 0, error_str, __FILE__, __LINE__);
  }

  glAttachShader(*program, shader);
  glDeleteShader(shader);

  // PIXEL SHADER
  text_ptr[0] = ps_text.c_str();

  shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(shader, 1, text_ptr, nullptr);
  glCompileShader(shader);

  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, 512, nullptr, info_log);

    ct::string error_str = "Fragment shader compilation failed:\n";
    error_str += info_log;
    cu::AssertError(success > 0, error_str, __FILE__, __LINE__);
  }

  glAttachShader(*program, shader);
  glDeleteShader(shader);

  // LINK
  glLinkProgram(*program);
}
}  // namespace lib_graphics
