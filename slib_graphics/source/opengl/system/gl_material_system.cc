#include "gl_material_system.h"
#include <GL/glew.h>
#include "core_utilities.h"
#include "engine_core.h"
#include "entity_manager.h"
#include "graphics_commands.h"
#include "light.h"
#include "light_system.h"
#include "system_manager.h"
#include "window.h"

namespace lib_graphics {
GlMaterialSystem::GlMaterialSystem(lib_core::EngineCore *engine)
    : MaterialSystem(engine) {}

GlMaterialSystem::~GlMaterialSystem() {
  for (auto &shader : shaders_) glDeleteProgram(shader.second);

  ct::dyn_array<GLuint> inds;
  for (auto &tex : textures_) inds.push_back(tex.second.first);
  glDeleteTextures(GLsizei(inds.size()), inds.data());

  inds.clear();
  for (auto &fb : frame_buffers_) {
    inds.push_back(fb.second.fbo);
    glDeleteRenderbuffers(GLsizei(fb.second.rend_buffers.size()),
                          fb.second.rend_buffers.data());
  }
  glDeleteFramebuffers(GLsizei(inds.size()), inds.data());

  shadow_frame_buffers_2d_.clear();
  shadow_frame_buffers_3d_.clear();
}

void GlMaterialSystem::LoadTexture2D(Texture &texture, bool force) {
  auto it = texture_map_.find(texture.name_hash);
  size_t id;
  if (it == texture_map_.end())
    id = GetTextureId(texture.name_hash);
  else
    id = it->second;

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  GLuint tex_id;
  glGenTextures(1, &tex_id);
  glBindTexture(GL_TEXTURE_2D, tex_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  float aniso = 0.0f;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);

  int format = GL_RGB;
  switch (texture.nr_channels) {
    case 1:
      format = GL_R;
      break;
    case 2:
      format = GL_RG;
      break;
    case 3:
      format = GL_RGB;
      break;
    case 4:
      format = GL_RGBA;
      break;
    default:
      format = GL_RGB;
      break;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, format, int(texture.dim.first),
               int(texture.dim.second), 0, format, GL_UNSIGNED_BYTE,
               texture.data.data());
  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);

  textures_[id] = {tex_id, kTexture2D};
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);
}

/**
 * Cube map face order:
 *	right, left, top, bottom, back, front
 */
void GlMaterialSystem::LoadTexture3D(ct::dyn_array<Texture *> &faces,
                                     size_t name_hash, bool force) {
  auto it = texture_map_.find(name_hash);
  size_t id;
  if (it == texture_map_.end())
    id = GetTextureId(name_hash);
  else
    id = it->second;

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  GLuint texture_id;
  glGenTextures(1, &texture_id);
  glActiveTexture(GL_TEXTURE0);

  glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);
  for (GLuint i = 0; i < faces.size(); i++) {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                 int(faces[i]->dim.first), int(faces[i]->dim.second), 0, GL_RGB,
                 GL_UNSIGNED_BYTE, faces[i]->data.data());
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

  textures_[id] = {texture_id, kTexture3D};
  texture_3d_[id] = {name_hash, faces};

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);
}

bool GlMaterialSystem::ForceMaterial(
    Material &mat, ct::dyn_array<std::pair<lib_core::Entity, Light>> *lights) {
  bool fully_set = true;

  auto shader_it = shaders_.find(mat.shader);
  if (shader_it == shaders_.end()) return false;

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  auto shader_program = shader_it->second;

  if (mat.shader != current_shader_id_) {
    glUseProgram(shader_program);
    current_shader_ = shader_program;
    current_shader_id_ = mat.shader;
  }

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  for (auto &p : mat.float_vec3) {
    auto vec_loc = glGetUniformLocation(shader_program, p.first.c_str());
    if (vec_loc != -1) glUniform3fv(vec_loc, 1, &p.second[0]);
  }

  for (auto &p : mat.float_var) {
    auto val_loc = glGetUniformLocation(shader_program, p.first.c_str());
    if (val_loc != -1) glUniform1f(val_loc, p.second);
  }

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  int tex_id = 0;
  if (lights) {
    int i = 0;
    for (auto &light : *lights) {
      GLint light_loc;
      if (light.second.type == Light::kPoint) {
        light_loc = glGetUniformLocation(
            shader_program,
            ("light_position[" + std::to_string(i) + "]").c_str());
        if (light_loc == -1) break;
        glUniform3fv(light_loc, 1, (float *)&light.second.data_pos);

        glUniform3fv(glGetUniformLocation(
                         shader_program,
                         ("light_coeff[" + std::to_string(i) + "]").c_str()),
                     1, &light.second.constant);

        glUniform1f(glGetUniformLocation(
                        shader_program,
                        ("light_radius[" + std::to_string(i) + "]").c_str()),
                    light.second.max_radius);
      } else if (light.second.type == Light::kDir) {
        light_loc = glGetUniformLocation(
            shader_program,
            ("light_directions[" + std::to_string(i) + "]").c_str());
        if (light_loc)
          glUniform3fv(light_loc, 1, (float *)&light.second.data_dir);
      }

      glUniform3fv(glGetUniformLocation(
                       shader_program,
                       ("light_color[" + std::to_string(i) + "]").c_str()),
                   1, (float *)&light.second.color);

      if (light.second.cast_shadows) {
        if (light.second.type == Light::kPoint) {
          glActiveTexture(GL_TEXTURE0 + tex_id);
          auto depth_frame_buffer = GetFrameBuffer(
              Get3DShadowFrameBuffer(light.second.shadow_resolutions[0]));

          glBindTexture(GL_TEXTURE_CUBE_MAP,
                        textures_[depth_frame_buffer->textures[0].id].first);
          auto tex_loc = glGetUniformLocation(
              shader_program, ("depth_map[" + std::to_string(i) + "]").c_str());
          glUniform1i(tex_loc, tex_id);
          auto far_loc = glGetUniformLocation(
              shader_program, ("far_plane[" + std::to_string(i) + "]").c_str());
          glUniform1f(far_loc, light.second.max_radius);
          ++tex_id;

          used_textures_.insert(depth_frame_buffer->textures[0].id);
        } else if (light.second.type == Light::kDir) {
          for (int ii = 0; ii < 3; ++ii) {
            glActiveTexture(GL_TEXTURE0 + tex_id);
            auto depth_frame_buffer = GetFrameBuffer(
                Get2DShadowFrameBuffer(light.second.shadow_resolutions[ii]));

            glBindTexture(GL_TEXTURE_2D,
                          textures_[depth_frame_buffer->textures[0].id].first);
            auto tex_loc = glGetUniformLocation(
                shader_program,
                ("depth_map[" + std::to_string(ii) + "]").c_str());
            glUniform1i(tex_loc, tex_id);
            auto cascade_loc = glGetUniformLocation(
                shader_program,
                ("cascade_depth[" + std::to_string(ii) + "]").c_str());
            glUniform1f(cascade_loc, light.second.view_depth[ii]);
            ++tex_id;

            used_textures_.insert(depth_frame_buffer->textures[0].id);
          }
        }

        auto shadow_loc = glGetUniformLocation(
            shader_program,
            ("shadow_caster[" + std::to_string(i) + "]").c_str());
        glUniform1i(shadow_loc, 1);
      } else {
        auto shadow_loc = glGetUniformLocation(
            shader_program,
            ("shadow_caster[" + std::to_string(i) + "]").c_str());
        glUniform1i(shadow_loc, 0);
      }
      ++i;
    }
    glUniform1i(glGetUniformLocation(shader_program, "nr_lights"), GLuint(i));
  } else {
    glUniform1i(glGetUniformLocation(shader_program, "nr_lights"), GLuint(0));
  }

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  for (int i = 0; i < mat.textures.size(); i++) {
    auto susp_it = suspended_textures_.find(mat.textures[i].id);
    if (susp_it != suspended_textures_.end()) {
      auto source_it = texture_source_.find(mat.textures[i].id);
      if (source_it != texture_source_.end()) {
        LoadTexture2D(*source_it->second, true);
        suspended_textures_.erase(susp_it);
      }
    }

    auto it = textures_.find(mat.textures[i].id);
    std::pair<GLuint, TextureType> *ogl_tex_id;
    if (it == textures_.end()) {
      fully_set = false;
      ogl_tex_id = &textures_[lib_core::EngineCore::stock_texture];
    } else {
      used_textures_.insert(it->first);
      ogl_tex_id = &it->second;
    }

    auto gl_err = glGetError();
    cu::AssertError(gl_err == GL_NO_ERROR, "OpenGL error", __FILE__, __LINE__);

    auto tex_loc =
        glGetUniformLocation(shader_program, mat.textures[i].name.c_str());
    if (tex_loc != -1) {
      glActiveTexture(GL_TEXTURE0 + tex_id);
      gl_err = glGetError();
      cu::AssertError(gl_err == GL_NO_ERROR,
                      "OpenGL error: " + std::to_string(gl_err) + ":" +
                          std::to_string(tex_id),
                      __FILE__, __LINE__);

      switch (ogl_tex_id->second) {
        case kTexture2D:
          glBindTexture(GL_TEXTURE_2D, ogl_tex_id->first);
          while (glGetError() != GL_NO_ERROR) {
          }
          gl_err = glGetError();
          cu::AssertError(gl_err == GL_NO_ERROR,
                          "OpenGL error: " + std::to_string(gl_err) + ":" +
                              std::to_string(ogl_tex_id->first),
                          __FILE__, __LINE__);
          break;
        case kTexture3D:
          glBindTexture(GL_TEXTURE_CUBE_MAP, ogl_tex_id->first);
          break;
      }
      glUniform1i(tex_loc, tex_id);
      ++tex_id;
    }
  }
  glActiveTexture(GL_TEXTURE0);

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  return fully_set;
}

size_t GlMaterialSystem::GetStockShaderId(ShaderType type) {
  return stock_shaders_.GetShaderId(type);
}

size_t GlMaterialSystem::GetStockShaderIdConvert(size_t id) {
  return stock_shaders_.DefToForId(id);
}

void GlMaterialSystem::RebuildTextures() {
  for (auto &tex : textures_) {
    auto source = texture_source_.find(tex.first);
    if (source == texture_source_.end()) continue;

    switch (tex.second.second) {
      case TextureType::kTexture2D:
        LoadTexture2D(*source->second, true);
        break;
      case TextureType::kTexture3D:
        break;
    }
  }

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  for (auto &tex : texture_3d_)
    LoadTexture3D(tex.second.second, tex.second.first, true);

  for (auto &frame_buffer : frame_buffers_) {
    if (frame_buffer.second.purged) {
      glGenFramebuffers(1, &frame_buffer.second.fbo);

      PushFrameBuffer(frame_buffer.first);

      GLuint tex_gluid;
      glGenTextures(1, &tex_gluid);
      auto *tex = &textures_[frame_buffer.second.textures[0].id];
      tex->first = tex_gluid;

      const GLuint tex_width = GLuint(frame_buffer.second.dim.first),
                   tex_height = GLuint(frame_buffer.second.dim.second);
      glBindTexture(GL_TEXTURE_2D, tex->first);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_width, tex_height, 0, GL_RGB,
                   GL_UNSIGNED_BYTE, NULL);
      glBindTexture(GL_TEXTURE_2D, 0);

      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, tex->first, 0);

      PopFrameBuffer();
      frame_buffer.second.purged = false;

      issue_command(frame_buffer.second.last_command);
    }
  }

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);
}

void GlMaterialSystem::PurgeGpuResources() {
  GLenum error;
  for (auto &tex : textures_) {
    glDeleteTextures(1, &tex.second.first);
    error = glGetError();
    cu::AssertError(error == GL_NO_ERROR,
                    "OpenGL error: " + std::to_string(error), __FILE__,
                    __LINE__);
  }
  for (auto &shader : shaders_) {
    if (shader.second != -1) {
      glDeleteProgram(shader.second);
      shader.second = -1;
    }
    error = glGetError();
    cu::AssertError(error == GL_NO_ERROR,
                    "OpenGL error: " + std::to_string(error), __FILE__,
                    __LINE__);
  }
  for (auto &light_map : shadow_frame_buffers_2d_) {
    ForceFreeFramebuffer(light_map.second);
    error = glGetError();
    cu::AssertError(error == GL_NO_ERROR,
                    "OpenGL error: " + std::to_string(error), __FILE__,
                    __LINE__);
  }
  for (auto &light_map : shadow_frame_buffers_3d_) {
    ForceFreeFramebuffer(light_map.second);
    error = glGetError();
    cu::AssertError(error == GL_NO_ERROR,
                    "OpenGL error: " + std::to_string(error), __FILE__,
                    __LINE__);
  }
  shadow_frame_buffers_2d_.clear();
  shadow_buffer_use_2d_.clear();
  shadow_frame_buffers_3d_.clear();
  shadow_buffer_use_3d_.clear();

  for (auto &frame_buffer : frame_buffers_) {
    ForceFreeFramebuffer(frame_buffer.first, false);
    frame_buffer.second.purged = true;
  }
}

void GlMaterialSystem::CreateFrameBuffer(size_t fb_id, size_t dim_x,
                                         size_t dim_y) {
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  FrameBuffer frame_buffer;
  frame_buffer.dim = {dim_x, dim_y};
  glGenFramebuffers(1, &frame_buffer.fbo);
  frame_buffers_[fb_id] = std::move(frame_buffer);

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);
}

bool GlMaterialSystem::PushFrameBuffer(size_t draw_buffer, size_t read_buffer) {
  auto it_draw = frame_buffers_.find(draw_buffer);
  cu::AssertWarning(it_draw != frame_buffers_.end(), "Frame buffer not found.",
                    __FILE__, __LINE__);

  if (it_draw != frame_buffers_.end()) {
    auto it_read = frame_buffers_.find(read_buffer);
    cu::AssertWarning(it_read != frame_buffers_.end(),
                      "Frame buffer not found.", __FILE__, __LINE__);
    if (it_read == frame_buffers_.end()) return false;

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, it_draw->second.fbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, it_read->second.fbo);
    glViewport(0, 0, int(it_draw->second.dim.first),
               int(it_draw->second.dim.second));

    frame_buffer_stack_.push_back({draw_buffer, read_buffer});
    return true;
  }
  return false;
}

bool GlMaterialSystem::PushFrameBuffer(size_t draw_buffer) {
  auto it_draw = frame_buffers_.find(draw_buffer);
  cu::AssertWarning(it_draw != frame_buffers_.end(), "Frame buffer not found.",
                    __FILE__, __LINE__);

  if (it_draw != frame_buffers_.end()) {
    cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                    __LINE__);
    glBindFramebuffer(GL_FRAMEBUFFER, it_draw->second.fbo);
    glViewport(0, 0, int(it_draw->second.dim.first),
               int(it_draw->second.dim.second));
    cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                    __LINE__);

    frame_buffer_stack_.push_back({draw_buffer, -1});
    return true;
  }
  return false;
}

void GlMaterialSystem::PopFrameBuffer() {
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);
  frame_buffer_stack_.pop_back();
  while (!frame_buffer_stack_.empty()) {
    auto it_draw = frame_buffers_.find(frame_buffer_stack_.back().first);
    cu::AssertWarning(it_draw != frame_buffers_.end(),
                      "Frame buffer not found.", __FILE__, __LINE__);

    auto it_read = frame_buffers_.find(frame_buffer_stack_.back().second);
    if (it_draw != frame_buffers_.end()) {
      if (it_read == frame_buffers_.end()) {
        glBindFramebuffer(GL_FRAMEBUFFER, it_draw->second.fbo);
        glViewport(0, 0, int(it_draw->second.dim.first),
                   int(it_draw->second.dim.second));
      } else {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, it_draw->second.fbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, it_read->second.fbo);
        glViewport(0, 0, int(it_draw->second.dim.first),
                   int(it_draw->second.dim.second));
      }
      break;
    } else
      frame_buffer_stack_.pop_back();
  }

  if (frame_buffer_stack_.empty()) {
    auto window_dim = engine_->GetWindow()->GetWindowDim();
    glViewport(0, 0, window_dim.first, window_dim.second);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);
}

void GlMaterialSystem::CreateTexture2D(size_t id) {
  GLuint tex_gluid;
  glGenTextures(1, &tex_gluid);
  textures_[id] = {tex_gluid, TextureType::kTexture2D};
}

void GlMaterialSystem::CreateDeferredRendererFrameBuffer(
    CreateDeferedFrameBufferCommand &command) {
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  CreateFrameBuffer(command.FrameBufferId(), command.width, command.height);
  auto &fbuffer = frame_buffers_[command.FrameBufferId()];

  PushFrameBuffer(command.FrameBufferId());

  CreateTexture2D(command.PositionTextureId());
  auto *tex = &textures_[command.PositionTextureId()];
  glBindTexture(GL_TEXTURE_2D, tex->first);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, int(command.width),
               int(command.height), 0, GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         tex->first, 0);
  fbuffer.textures.push_back({command.PositionTextureId(), "g_position"});

  CreateTexture2D(command.NormalTextureId());
  tex = &textures_[command.NormalTextureId()];
  glBindTexture(GL_TEXTURE_2D, tex->first);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, int(command.width),
               int(command.height), 0, GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                         tex->first, 0);
  fbuffer.textures.push_back({command.NormalTextureId(), "g_normal"});

  CreateTexture2D(command.AlbedoTextureId());
  tex = &textures_[command.AlbedoTextureId()];
  glBindTexture(GL_TEXTURE_2D, tex->first);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, int(command.width),
               int(command.height), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D,
                         tex->first, 0);
  fbuffer.textures.push_back({command.AlbedoTextureId(), "g_albedo"});

  CreateTexture2D(command.RmeTextureId());
  tex = &textures_[command.RmeTextureId()];
  glBindTexture(GL_TEXTURE_2D, tex->first);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, int(command.width),
               int(command.height), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D,
                         tex->first, 0);
  fbuffer.textures.push_back({command.RmeTextureId(), "g_rma"});

  GLuint attachments[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                           GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
  glDrawBuffers(4, attachments);

  CreateTexture2D(command.DepthTextureId());
  tex = &textures_[command.DepthTextureId()];
  glBindTexture(GL_TEXTURE_2D, tex->first);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, int(command.width),
               int(command.height), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

  /*glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, int(command.width),
               int(command.height), 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
               NULL);*/

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex->first, 0);
  /*glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
     tex->first, 0);*/

  fbuffer.textures.push_back({command.DepthTextureId(), "g_depth"});

  PopFrameBuffer();

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);
}

void GlMaterialSystem::CreateHdrFrameBuffer(
    CreateHdrFrameBufferCommand &command) {
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  CreateFrameBuffer(command.FrameBufferId(), command.width, command.height);
  auto &fbuffer = frame_buffers_[command.FrameBufferId()];

  PushFrameBuffer(command.FrameBufferId());

  CreateTexture2D(command.HdrTextureId());
  auto tex = &textures_[command.HdrTextureId()];
  glBindTexture(GL_TEXTURE_2D, tex->first);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, int(command.width),
               int(command.height), 0, GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         tex->first, 0);
  fbuffer.textures.push_back({command.HdrTextureId(), "hdr_buffer"});

  CreateTexture2D(command.DepthTextureId());
  tex = &textures_[command.DepthTextureId()];
  glBindTexture(GL_TEXTURE_2D, tex->first);
  /*glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, int(command.width),
               int(command.height), 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
               NULL);*/
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, int(command.width),
               int(command.height), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  /*glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
     tex->first, 0);*/
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex->first, 0);
  fbuffer.textures.push_back({command.DepthTextureId(), "depth_buffer"});

  PopFrameBuffer();

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);
}

void GlMaterialSystem::CompileShaders() {
  stock_shaders_.CompileShaders(this);
  lib_graphics::Material mat;
  mat.shader = GetStockShaderId(lib_graphics::MaterialSystem::kPbrUntextured);
  materials_[size_t(std::numeric_limits<size_t>::max)] = mat;
}

uint32_t GlMaterialSystem::GetCurrentShader() { return current_shader_; }

GLuint GlMaterialSystem::GetShaderId(size_t id) { return shaders_[id]; }

void GlMaterialSystem::DrawUpdate(lib_graphics::Renderer *renderer,
                                  lib_gui::TextSystem *text_renderer) {
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  auto blur_fb_command = g_sys_mgr.GetCommands<CreateBlurFrameBufferCommand>();
  if (blur_fb_command && !blur_fb_command->empty()) {
    for (auto &c : *blur_fb_command) CreateBlurTargets(c);
    blur_fb_command->clear();
  }

  auto hdr_fb_command = g_sys_mgr.GetCommands<CreateHdrFrameBufferCommand>();
  if (hdr_fb_command && !hdr_fb_command->empty()) {
    for (auto &c : *hdr_fb_command) CreateHdrFrameBuffer(c);
    hdr_fb_command->clear();
  }

  auto deferred_fb_command =
      g_sys_mgr.GetCommands<CreateDeferedFrameBufferCommand>();
  if (deferred_fb_command && !deferred_fb_command->empty()) {
    for (auto &c : *deferred_fb_command) CreateDeferredRendererFrameBuffer(c);
    deferred_fb_command->clear();
  }

  ++clean_counter_;

  if (clean_counter_ > 500) {
    rem_fb_vec.clear();
    for (auto &l : shadow_frame_buffers_2d_) {
      auto it = shadow_buffer_use_2d_.find(l.first);
      if (it == shadow_buffer_use_2d_.end()) {
        ForceFreeFramebuffer(l.second, true);
        rem_fb_vec.push_back(l.first);
      }
    }

    for (auto &r : rem_fb_vec) shadow_frame_buffers_2d_.erase(r);
    shadow_buffer_use_2d_.clear();

    rem_fb_vec.clear();
    for (auto &l : shadow_frame_buffers_3d_) {
      auto it = shadow_buffer_use_3d_.find(l.first);
      if (it == shadow_buffer_use_3d_.end()) {
        ForceFreeFramebuffer(l.second, true);
        rem_fb_vec.push_back(l.first);
      }
    }

    for (auto &p : textures_) {
      if (p.second.second == kTexture3D ||
          collection_exempt_.find(p.first) != collection_exempt_.end())
        continue;

      if (used_textures_.find(p.first) == used_textures_.end()) {
        if (suspended_textures_.find(p.first) == suspended_textures_.end()) {
          glDeleteTextures(1, &p.second.first);
          suspended_textures_.insert(p.first);
        }
      }
    }
    used_textures_.clear();

    for (auto &r : rem_fb_vec) shadow_frame_buffers_3d_.erase(r);

    clean_counter_ = 0;
    shadow_buffer_use_3d_.clear();
  }

  size_t tex_id;
  if (add_textures_.try_pop(tex_id)) LoadTexture2D(*texture_source_[tex_id]);

  auto add_material_commands = g_sys_mgr.GetCommands<AddMaterialCommand>();
  if (add_material_commands) {
    for (auto &c : *add_material_commands)
      materials_[c.MaterialId()] = std::move(c.material);
    add_material_commands->clear();
  }

  auto update_material_command = g_sys_mgr.GetCommands<UpdateMaterialCommand>();
  if (update_material_command && !update_material_command->empty()) {
    for (auto &c : *update_material_command)
      materials_[c.material_id] = std::move(c.material);
    update_material_command->clear();
  }

  auto remove_material_commands =
      g_sys_mgr.GetCommands<RemoveMaterialCommand>();
  if (remove_material_commands) {
    for (auto &c : *remove_material_commands) {
      auto it = materials_.find(c.material_id);
      if (it != materials_.end()) {
        for (auto &fb : it->second.frame_buffers)
          issue_command(lib_graphics::FreeTextureFrameBufferCommand(fb));
        materials_.erase(it);
      }
    }
    remove_material_commands->clear();
  }

  ct::dyn_array<size_t> tex_ids;
  if (add_texture_3d_.try_pop(tex_ids)) {
    ct::string name = "";
    ct::dyn_array<Texture *> tex_faces;
    for (auto &tex : tex_ids) {
      tex_faces.push_back(texture_source_[tex].get());
      name += name_hashes_[tex_faces.back()->name_hash];
    }

    LoadTexture3D(tex_faces, std::hash<ct::string>{}(name));
  }

  auto free_fb_commands =
      g_sys_mgr.GetCommands<FreeTextureFrameBufferCommand>();
  if (free_fb_commands && !free_fb_commands->empty()) {
    for (auto &c : *free_fb_commands) {
      auto it = frame_buffers_.find(c.frame_buffer_id);
      if (it == frame_buffers_.end())
        removed_fbs_.insert(c.frame_buffer_id);
      else
        available_framebuffers_.emplace_back(c.frame_buffer_id);
    }
    free_fb_commands->clear();
  }

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  auto fb_commands = g_sys_mgr.GetCommands<CreateTextureFrameBufferCommand>();
  if (fb_commands && !fb_commands->empty()) {
    while (!removed_fbs_.empty() &&
           removed_fbs_.find(fb_commands->front().FrameBufferId()) !=
               removed_fbs_.end()) {
      removed_fbs_.erase(fb_commands->front().FrameBufferId());
      fb_commands->pop_front();
    }

    if (!fb_commands->empty()) {
      Create2DTextureTarget(fb_commands->front());
      fb_commands->pop_front();
    }
  }

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  auto add_shader_commands = g_sys_mgr.GetCommands<AddShaderCommand>();
  if (add_shader_commands) {
    for (auto &c : *add_shader_commands) CompileShaderRecource(c);
    add_shader_commands->clear();
  }

  auto remove_shader_commands = g_sys_mgr.GetCommands<RemoveShaderCommand>();
  if (remove_shader_commands) {
    for (auto &c : *remove_shader_commands) {
      auto it = shaders_.find(c.shader_id);
      if (it != shaders_.end()) {
        if (it->second != -1) glDeleteProgram(it->second);
        shaders_.erase(it);
      }
    }
    remove_shader_commands->clear();
  }

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  auto render_commands = g_sys_mgr.GetCommands<RenderToTextureCommand>();
  if (render_commands && !render_commands->empty()) {
    auto fb = frame_buffers_.find(render_commands->front().frame_buffer);
    if (fb != frame_buffers_.end()) {
      PushFrameBuffer(fb->first);
      render_commands->front().render_function(renderer, text_renderer);
      PopFrameBuffer();

      fb->second.last_command = render_commands->front();

      render_commands->pop_front();
      mipmap_generation_queue_.emplace_back(fb->second.textures[0].id);
    } else
      render_commands->pop_front();
  }

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  if (!mipmap_generation_queue_.empty()) {
    auto texture_it = textures_.find(mipmap_generation_queue_.back());
    if (texture_it != textures_.end()) {
      glBindTexture(GL_TEXTURE_2D, texture_it->second.first);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                      GL_LINEAR_MIPMAP_LINEAR);

      float aniso = 0.0f;
      glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);

      glGenerateMipmap(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, 0);
      mipmap_generation_queue_.pop_back();
    }
  }

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);
}

size_t GlMaterialSystem::CreateCubeMapShadowTarget(size_t res) {
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  auto base_id = g_sys_mgr.GenerateResourceIds(2);

  auto frame_buffer_id = base_id;
  CreateFrameBuffer(frame_buffer_id, res, res);
  auto &fbuffer = frame_buffers_[frame_buffer_id];

  PushFrameBuffer(frame_buffer_id);

  CreateTexture2D(base_id + 1);
  auto tex = &textures_[base_id + 1];

  const GLuint SHADOW_WIDTH = GLuint(res), SHADOW_HEIGHT = GLuint(res);
  glBindTexture(GL_TEXTURE_CUBE_MAP, tex->first);
  for (GLuint i = 0; i < 6; ++i)
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                 NULL);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex->first, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  PopFrameBuffer();

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  fbuffer.textures.push_back({base_id + 1, "s_cube_map"});
  return frame_buffer_id;
}

size_t GlMaterialSystem::Create2DShadowTarget(size_t res_x, size_t res_y) {
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  auto base_id = g_sys_mgr.GenerateResourceIds(2);

  auto frame_buffer_id = base_id;
  CreateFrameBuffer(frame_buffer_id, res_x, res_y);
  auto &fbuffer = frame_buffers_[frame_buffer_id];

  PushFrameBuffer(frame_buffer_id);

  CreateTexture2D(base_id + 1);
  auto tex = &textures_[base_id + 1];
  const GLuint SHADOW_WIDTH = GLuint(res_x), SHADOW_HEIGHT = GLuint(res_y);
  glBindTexture(GL_TEXTURE_2D, tex->first);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH,
               SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
                  GL_COMPARE_REF_TO_TEXTURE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         tex->first, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  PopFrameBuffer();

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  fbuffer.textures.push_back({base_id + 1, "s_shadow_map"});
  return frame_buffer_id;
}

void GlMaterialSystem::Create2DTextureTarget(
    CreateTextureFrameBufferCommand &fb_command) {
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  if (!available_framebuffers_.empty()) {
    // Replace frame buffer id
    auto &new_fb = frame_buffers_[fb_command.FrameBufferId()];
    new_fb = frame_buffers_[available_framebuffers_.back()];
    frame_buffers_.erase(available_framebuffers_.back());

    // Replace texture id
    auto &new_tex = textures_[fb_command.TextureId()];
    new_tex = textures_[new_fb.textures[0].id];
    textures_.erase(new_fb.textures[0].id);
    new_fb.textures[0].id = fb_command.TextureId();

    available_framebuffers_.pop_back();
    return;
  }

  CreateFrameBuffer(fb_command.FrameBufferId(), fb_command.width,
                    fb_command.height);

  auto &fbuffer = frame_buffers_[fb_command.FrameBufferId()];

  PushFrameBuffer(fb_command.FrameBufferId());
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);
  CreateTexture2D(fb_command.TextureId());
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);
  auto *tex = &textures_[fb_command.TextureId()];
  const GLuint tex_width = GLuint(fb_command.width),
               tex_height = GLuint(fb_command.height);
  glBindTexture(GL_TEXTURE_2D, tex->first);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_width, tex_height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, NULL);
  glBindTexture(GL_TEXTURE_2D, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         tex->first, 0);

  PopFrameBuffer();

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  collection_exempt_.insert(fb_command.TextureId());
  fbuffer.textures.push_back({fb_command.TextureId(), "tex_map"});
}

void GlMaterialSystem::CreateBlurTargets(
    CreateBlurFrameBufferCommand &command) {
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  size_t fb_ids[2];
  fb_ids[0] = command.FrameBuffer1Id();
  fb_ids[1] = command.FrameBuffer2Id();

  size_t tex_ids[2];
  tex_ids[0] = command.Texture1Id();
  tex_ids[1] = command.Texture2Id();

  for (GLuint i = 0; i < 2; i++) {
    CreateFrameBuffer(fb_ids[i], command.width, command.height);
    PushFrameBuffer(fb_ids[i]);

    CreateTexture2D(tex_ids[i]);
    auto *tex = &textures_[tex_ids[i]];
    glBindTexture(GL_TEXTURE_2D, tex->first);
    glTexImage2D(GL_TEXTURE_2D, 0, command.texture_type, int(command.width),
                 int(command.height), 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (command.shadow)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
                      GL_COMPARE_REF_TO_TEXTURE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           tex->first, 0);
    PopFrameBuffer();
  }

  auto &fbuffer_1 = frame_buffers_[command.FrameBuffer1Id()];
  auto &fbuffer_2 = frame_buffers_[command.FrameBuffer2Id()];
  fbuffer_1.textures.push_back({tex_ids[0], "blur_tex"});
  fbuffer_2.textures.push_back({tex_ids[1], "blur_tex"});

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);
}

void GlMaterialSystem::ForceFreeFramebuffer(size_t frame_buffer_id,
                                            bool erase) {
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  auto it = frame_buffers_.find(frame_buffer_id);
  if (it != frame_buffers_.end()) {
    for (auto &tex : it->second.textures) {
      auto texture_it = textures_.find(tex.id);
      if (texture_it != textures_.end()) {
        glDeleteTextures(1, &texture_it->second.first);
        if (erase) textures_.erase(texture_it);
      }
    }

    for (auto &rbo : it->second.rend_buffers) glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &it->second.fbo);
    if (erase) frame_buffers_.erase(it);
  }

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);
}

size_t GlMaterialSystem::Get2DShadowFrameBuffer(size_t res) {
  auto it = shadow_frame_buffers_2d_.find(res);
  if (it == shadow_frame_buffers_2d_.end()) {
    shadow_frame_buffers_2d_[res] = Create2DShadowTarget(res, res);
    it = shadow_frame_buffers_2d_.find(res);
  }
  shadow_buffer_use_2d_.insert(res);
  return it->second;
}

size_t GlMaterialSystem::Get3DShadowFrameBuffer(size_t res) {
  auto it = shadow_frame_buffers_3d_.find(res);
  if (it == shadow_frame_buffers_3d_.end()) {
    shadow_frame_buffers_3d_[res] = CreateCubeMapShadowTarget(res);
    it = shadow_frame_buffers_3d_.find(res);
  }
  shadow_buffer_use_3d_.insert(res);
  return it->second;
}

GLuint GlMaterialSystem::TextureById(size_t id) {
  auto it = textures_.find(id);
  if (it != textures_.end()) return it->second.first;
  return 0;
}

void GlMaterialSystem::CompileShaderRecource(AddShaderCommand &code) {
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  GLuint vertex_shader;
  vertex_shader = glCreateShader(GL_VERTEX_SHADER);

  const char *code_char = code.vert_shader.c_str();
  glShaderSource(vertex_shader, 1, &code_char, NULL);
  glCompileShader(vertex_shader);

  GLint success;
  GLchar info_log[512];
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);

  if (!success) {
    glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);

    ct::string error_str = "Vertex shader compilation failed:\n";
    error_str += info_log;
    cu::AssertError(success > 0, error_str, __FILE__, __LINE__);
  }

  GLuint fragment_shader;
  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

  ct::string frag_code = code.frag_shader;
  if (frag_code.empty()) {
    frag_code =
        "#version 330 core\n"
        "void main()\n"
        "{\n"
        "}";
  }

  code_char = frag_code.c_str();
  glShaderSource(fragment_shader, 1, &code_char, NULL);
  glCompileShader(fragment_shader);

  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);

  if (!success) {
    glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);

    ct::string error_str = "Fragment shader compilation failed:\n";
    error_str += info_log;
    cu::AssertError(success > 0, error_str, __FILE__, __LINE__);
  }

  GLuint geometry_shader = 0;
  if (!code.geom_shader.empty()) {
    geometry_shader = glCreateShader(GL_GEOMETRY_SHADER);

    code_char = code.geom_shader.c_str();
    glShaderSource(geometry_shader, 1, &code_char, NULL);
    glCompileShader(geometry_shader);

    glGetShaderiv(geometry_shader, GL_COMPILE_STATUS, &success);

    if (!success) {
      glGetShaderInfoLog(geometry_shader, 512, NULL, info_log);

      ct::string error_str = "Geometry shader compilation failed:\n";
      error_str += info_log;
      cu::AssertError(success > 0, error_str, __FILE__, __LINE__);
    }
  }

  GLuint shader_program;
  shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  if (!code.geom_shader.empty())
    glAttachShader(shader_program, geometry_shader);
  glLinkProgram(shader_program);

  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shader_program, 512, NULL, info_log);

    ct::string error_str = "Shader linking failed:\n";
    error_str += info_log;
    cu::AssertError(success > 0, error_str, __FILE__, __LINE__);
  }

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  if (!code.geom_shader.empty()) glDeleteShader(geometry_shader);

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error", __FILE__,
                  __LINE__);

  shaders_[code.ShaderId()] = shader_program;
}
}  // namespace lib_graphics
