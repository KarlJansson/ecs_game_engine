#include "gl_particle_system.h"
#include <GL/glew.h>
#include "graphics_commands.h"
#include "material_system.h"
#include "quaternion.h"
#include "transform.h"
#include "window.h"

namespace lib_graphics {
GlParticleSystem::GlParticleSystem(const lib_core::EngineCore *engine)
    : ParticleSystem(engine) {
  vert_shader_ =
      "#version 430 core\n"
      "layout(location = 0) in vec4 random;\n"
      "layout(location = 1) in vec3 position;\n"
      "layout(location = 2) in vec3 velocity;\n"
      "layout(location = 3) in vec2 corners;\n"
      "layout(location = 4) in float start_time;\n"

      "out vec2 tex_coord;\n"
      "out vec4 color;\n"

      "uniform mat4 view;\n"
      "uniform mat4 projection;\n"
      "uniform vec2 viewport_scale;\n"

      "uniform float current_time;\n"
      "uniform float duration;\n"
      "uniform float duration_randomness;\n"
      "uniform vec3 gravity;\n"
      "uniform float end_velocity;\n"
      "uniform vec4 min_color;\n"
      "uniform vec4 max_color;\n"

      "uniform vec2 rotate_speed;\n"
      "uniform vec2 start_size;\n"
      "uniform vec2 end_size;\n"

      "vec4 ComputeParticlePosition("
      "vec3 pos, vec3 velocity, float age, float normalizedAge)\n"
      "{\n"
      "  float startVelocity = length(velocity);\n"
      "  float endVelocity = startVelocity * end_velocity;\n"
      "  float velocityIntegral = startVelocity * normalizedAge + (endVelocity "
      "- startVelocity) * normalizedAge * normalizedAge / 2;\n"
      "  pos += normalize(velocity) * velocityIntegral * duration;\n"
      "  pos += gravity * age * normalizedAge;\n"
      "  return projection * view * vec4(pos,1.f);\n"
      "}\n"

      "float ComputeParticleSize(float randomValue, float normalizedAge)\n"
      "{\n"
      "  float startSize = mix(start_size.x, start_size.y, randomValue);\n"
      "  float endSize = mix(end_size.x, end_size.y, randomValue);\n"
      "  float size = mix(startSize, endSize, normalizedAge);\n"
      "  return size * projection[1][1];\n"
      "}\n"

      "vec4 ComputeParticleColor(float randomValue, float normalizedAge)\n"
      "{\n"
      "  vec4 color = mix(min_color, max_color, randomValue);\n"
      "  color = mix(color, max_color, normalizedAge)\n;"
      "  color.a *= normalizedAge * (1.0f - normalizedAge) * (1.0f - "
      "normalizedAge) * 6.7;\n"
      "  return color;\n"
      "}\n"

      "mat2 ComputeParticleRotation(float randomValue, float age)\n"
      "{\n"
      "  float rotateSpeed = mix(rotate_speed.x, rotate_speed.y, "
      "randomValue);\n"
      "  float rotation = rotateSpeed * age;\n"
      "  float c = cos(rotation);\n"
      "  float s = sin(rotation);\n"
      "  return mat2(c, -s, s, c);\n"
      "}\n"

      "void main()\n"
      "{\n"
      "  float age = current_time - start_time;\n"
      "  age *= 1.0f + random.x * duration_randomness;\n"
      "  float normalized_age = clamp(age / duration, 0.f, 1.f);\n"
      "  vec4 p = ComputeParticlePosition(position, velocity, age, "
      "normalized_age);\n"
      "  float size = ComputeParticleSize(random.y, normalized_age);\n"
      "  mat2 rotation = ComputeParticleRotation(random.w, age);\n"
      "  p.xy += corners * rotation * size * viewport_scale;\n"

      "  color = ComputeParticleColor(random.z, normalized_age);\n"
      "  tex_coord = (corners + 1) / 2;\n"

      "  gl_Position = p;\n"
      "}";

  frag_shader_ =
      "#version 430 core\n"
      "in vec2 tex_coord;\n"
      "in vec4 color;\n"

      "uniform sampler2D particle_texture;\n"
      "uniform sampler2D g_depth;\n"
      "uniform vec2 screen_dim;\n"

      "out vec4 FragColor;\n"

      "void main()\n"
      "{\n"
      "  vec2 frag_coord = gl_FragCoord.xy / screen_dim;\n"
      "  float depth = texture(g_depth, frag_coord).r;\n"
      "  vec4 out_color = texture(particle_texture, tex_coord) * color;\n"
      "  out_color.a *= clamp(abs(depth - gl_FragCoord.z) / gl_FragCoord.w, "
      "0.f, "
      "1.f) * 10.f;\n"
      "  FragColor = out_color;\n"
      "}\n";

  auto shader_cmd = lib_graphics::AddShaderCommand(vert_shader_, frag_shader_);
  shader_id_ = shader_cmd.ShaderId();
  issue_command(shader_cmd);

  particle_material_.shader = shader_id_;
  particle_material_.textures.push_back({0, "particle_texture"});
  particle_material_.textures.push_back({1, "g_depth"});

  add_callback_ = g_ent_mgr.RegisterAddComponentCallback<ParticleEmitter>(
      [&](lib_core::Entity e) { add_emitter_.insert(e); });
  remove_callback_ = g_ent_mgr.RegisterRemoveComponentCallback<ParticleEmitter>(
      [&](lib_core::Entity e) { remove_emitter_.insert(e); });

  std::random_device rng_dev;
  rng_engine_.seed(rng_dev());
}

void GlParticleSystem::DrawUpdate(lib_graphics::Renderer *renderer,
                                  lib_gui::TextSystem *text_renderer) {
  for (auto &e : add_emitter_) {
    auto emitter = g_ent_mgr.GetOldCbeR<ParticleEmitter>(e);
    if (emitter) AddParticleEmitter(e, *emitter);
  }
  add_emitter_.clear();

  for (auto &e : remove_emitter_) {
    auto it = emitter_data_.find(e);
    if (it != emitter_data_.end()) {
      if (it->second.mapped_ptr) {
        glBindBuffer(GL_ARRAY_BUFFER, it->second.vbo);
        glUnmapBuffer(GL_ARRAY_BUFFER);
      }
      glDeleteVertexArrays(1, &it->second.vao);
      glDeleteBuffers(1, &it->second.vbo);
      glDeleteBuffers(1, &it->second.ebo);
      emitter_data_.erase(it);
    }
  }
  remove_emitter_.clear();

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error.", __FILE__,
                  __LINE__);
}

void GlParticleSystem::FinalizeSystem() {
  g_ent_mgr.UnregisterAddComponentCallback<ParticleEmitter>(add_callback_);
  g_ent_mgr.UnregisterRemoveComponentCallback<ParticleEmitter>(
      remove_callback_);
  issue_command(lib_graphics::RemoveShaderCommand(shader_id_));

  for (auto &e : emitter_data_) {
    if (e.second.mapped_ptr) {
      glBindBuffer(GL_ARRAY_BUFFER, e.second.vbo);
      glUnmapBuffer(GL_ARRAY_BUFFER);
    }
    glDeleteVertexArrays(1, &e.second.vao);
    glDeleteBuffers(1, &e.second.vbo);
    glDeleteBuffers(1, &e.second.ebo);
  }
}

void GlParticleSystem::DrawParticleEmitter(lib_core::Entity entity,
                                           const lib_graphics::Camera &camera,
                                           const TextureDesc &depth_desc) {
  auto it = emitter_data_.find(entity);
  if (it != emitter_data_.end()) {
    auto screen_dim = engine_->GetWindow()->GetRenderDim();
    std::uniform_real_distribution<float> rng_dist(0.f, 1.f);
    auto emitter = g_ent_mgr.GetOldCbeW<ParticleEmitter>(entity);
    if (emitter) {
      auto transform = g_ent_mgr.GetOldCbeR<Transform>(entity);
      auto material_system = engine_->GetMaterial();
      auto &gpu_data = it->second;

      if (gpu_data.sync_obj) {
        GLenum wait_return = GL_UNSIGNALED;
        while (wait_return != GL_ALREADY_SIGNALED &&
               wait_return != GL_CONDITION_SATISFIED) {
          wait_return = glClientWaitSync(static_cast<GLsync>(gpu_data.sync_obj),
                                         GL_SYNC_FLUSH_COMMANDS_BIT, 1);
        }
        glDeleteSync(static_cast<GLsync>(gpu_data.sync_obj));
      }

      bool unmap_buffer = false;
      if (!gpu_data.mapped_ptr) {
        glBindBuffer(GL_ARRAY_BUFFER, gpu_data.vbo);
        gpu_data.mapped_ptr = static_cast<ParticleVertex *>(glMapBufferRange(
            GL_ARRAY_BUFFER, 0, gpu_data.buffer_size, GL_MAP_WRITE_BIT));
        unmap_buffer = true;
      }
      cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error.", __FILE__,
                      __LINE__);

      // Remove dead particles
      while (gpu_data.particle_count > 0 &&
             emitter->emitter_time -
                     gpu_data.mapped_ptr[gpu_data.head * 4].start_time >
                 emitter->particle_life) {
        --gpu_data.particle_count;
        ++gpu_data.head;
        if (gpu_data.head >= emitter->max_particles) gpu_data.head = 0;
      }

      // Add new particles
      gpu_data.delta_rest += emitter->emitter_time - gpu_data.last_time;
      auto add_count = int(gpu_data.delta_rest * emitter->emitting_speed);
      if (add_count > 0) {
        gpu_data.delta_rest -= add_count * (1.f / emitter->emitting_speed);

        size_t cursor =
            (gpu_data.head + gpu_data.particle_count) % emitter->max_particles;
        cursor *= 4;
        for (size_t i = 0; i < add_count; ++i) {
          if (gpu_data.particle_count != 0 && cursor == gpu_data.head) break;
          if (gpu_data.particle_count == emitter->max_particles) break;
          if (gpu_data.particle_emitted == emitter->max_particles) break;

          auto random_vec =
              lib_core::Vector4(rng_dist(rng_engine_), rng_dist(rng_engine_),
                                rng_dist(rng_engine_), rng_dist(rng_engine_));
          random_vec *= emitter->random_scale;
          for (size_t ii = 0; ii < 4; ++ii) {
            gpu_data.mapped_ptr[cursor + ii].random = random_vec;
            gpu_data.mapped_ptr[cursor + ii].start_time = emitter->emitter_time;
          }

          switch (emitter->emitt_type) {
            case ParticleEmitter::kCircle:
              AddParticleCircle(gpu_data.mapped_ptr + cursor, emitter,
                                transform);
              break;
            case ParticleEmitter::kCube:
              AddParticleCube(gpu_data.mapped_ptr + cursor, emitter, transform);
              break;
            default:
            case ParticleEmitter::kPoint:
              AddParticlePoint(gpu_data.mapped_ptr + cursor, emitter,
                               transform);
              break;
            case ParticleEmitter::kSphere:
              AddParticleSphere(gpu_data.mapped_ptr + cursor, emitter,
                                transform);
              break;
            case ParticleEmitter::kSquare:
              AddParticleSquare(gpu_data.mapped_ptr + cursor, emitter,
                                transform);
              break;
          }

          cursor += 4;
          if (cursor == emitter->max_particles * 4) cursor = 0;

          ++gpu_data.particle_count;
          if (!emitter->loop) ++gpu_data.particle_emitted;
        }
      }

      cu::AssertError(glGetError() == GL_NO_ERROR,
                      "OpenGL error - Draw Particles", __FILE__, __LINE__);
      if (unmap_buffer) {
        glUnmapBuffer(GL_ARRAY_BUFFER);
        gpu_data.mapped_ptr = nullptr;
      } else
        gpu_data.sync_obj = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
      auto err = glGetError();
      cu::AssertError(err == GL_NO_ERROR, "OpenGL error - Draw Particles",
                      __FILE__, __LINE__);

      gpu_data.last_time = emitter->emitter_time;

      // Render particles
      if (gpu_data.particle_count > 0) {
        particle_material_.textures[0].id = emitter->particle_texture;
        particle_material_.textures[1] = depth_desc;
        material_system->ForceMaterial(particle_material_);

        if (get_uniform_locations_) {
          auto current_shader = material_system->GetCurrentShader();
          shader_uniforms_[0] =
              glGetUniformLocation(current_shader, "current_time");
          shader_uniforms_[1] =
              glGetUniformLocation(current_shader, "duration");
          shader_uniforms_[2] =
              glGetUniformLocation(current_shader, "duration_randomness");
          shader_uniforms_[3] = glGetUniformLocation(current_shader, "gravity");
          shader_uniforms_[4] =
              glGetUniformLocation(current_shader, "end_velocity");
          shader_uniforms_[5] =
              glGetUniformLocation(current_shader, "min_color");
          shader_uniforms_[6] =
              glGetUniformLocation(current_shader, "max_color");
          shader_uniforms_[7] =
              glGetUniformLocation(current_shader, "rotate_speed");
          shader_uniforms_[8] =
              glGetUniformLocation(current_shader, "start_size");
          shader_uniforms_[9] =
              glGetUniformLocation(current_shader, "end_size");
          shader_uniforms_[10] = glGetUniformLocation(current_shader, "view");
          shader_uniforms_[11] =
              glGetUniformLocation(current_shader, "projection");
          shader_uniforms_[12] =
              glGetUniformLocation(current_shader, "viewport_scale");
          shader_uniforms_[13] =
              glGetUniformLocation(current_shader, "screen_dim");
          get_uniform_locations_ = false;
        }

        if (shader_uniforms_[0] != -1)
          glUniform1f(shader_uniforms_[0], emitter->emitter_time);
        if (shader_uniforms_[1] != -1)
          glUniform1f(shader_uniforms_[1], emitter->particle_life);
        if (shader_uniforms_[2] != -1) glUniform1f(shader_uniforms_[2], 0.4f);
        if (shader_uniforms_[3] != -1)
          glUniform3f(shader_uniforms_[3], emitter->gravity[0],
                      emitter->gravity[1], emitter->gravity[2]);
        if (shader_uniforms_[4] != -1)
          glUniform1f(shader_uniforms_[4], emitter->end_velocity);
        if (shader_uniforms_[5] != -1)
          glUniform4f(shader_uniforms_[5], emitter->start_color[0],
                      emitter->start_color[1], emitter->start_color[2],
                      emitter->start_color[3]);
        if (shader_uniforms_[6] != -1)
          glUniform4f(shader_uniforms_[6], emitter->end_color[0],
                      emitter->end_color[1], emitter->end_color[2],
                      emitter->end_color[3]);
        if (shader_uniforms_[7] != -1)
          glUniform2f(shader_uniforms_[7], emitter->rotate_speed[0],
                      emitter->rotate_speed[1]);
        if (shader_uniforms_[8] != -1)
          glUniform2f(shader_uniforms_[8], emitter->start_size[0],
                      emitter->start_size[1]);
        if (shader_uniforms_[9] != -1)
          glUniform2f(shader_uniforms_[9], emitter->end_size[0],
                      emitter->end_size[1]);

        if (shader_uniforms_[10] != -1)
          glUniformMatrix4fv(shader_uniforms_[10], 1, GL_FALSE,
                             camera.view_.data);
        if (shader_uniforms_[11] != -1)
          glUniformMatrix4fv(shader_uniforms_[11], 1, GL_FALSE,
                             camera.proj_.data);
        if (shader_uniforms_[12] != -1)
          glUniform2f(shader_uniforms_[12], .5f / camera.a_ratio_, -.5f);
        if (shader_uniforms_[13] != -1)
          glUniform2f(shader_uniforms_[13], float(screen_dim.first),
                      float(screen_dim.second));

        glBindVertexArray(gpu_data.vao);
        cu::AssertError(glGetError() == GL_NO_ERROR,
                        "OpenGL error - Draw Particles", __FILE__, __LINE__);
        if (gpu_data.head + gpu_data.particle_count > emitter->max_particles) {
          auto first_batch = emitter->max_particles - gpu_data.head - 1;
          glDrawElements(GL_TRIANGLES, GLsizei(first_batch * 6),
                         GL_UNSIGNED_INT,
                         (void *)(sizeof(GLuint) * gpu_data.head * 6));
          glDrawElements(GL_TRIANGLES,
                         GLsizei((gpu_data.particle_count - first_batch) * 6),
                         GL_UNSIGNED_INT, (void *)nullptr);
          cu::AssertError(glGetError() == GL_NO_ERROR,
                          "OpenGL error - Draw Particles", __FILE__, __LINE__);
        } else {
          glDrawElements(GL_TRIANGLES, GLsizei(gpu_data.particle_count * 6),
                         GL_UNSIGNED_INT,
                         (void *)(sizeof(GLuint) * gpu_data.head * 6));
          cu::AssertError(glGetError() == GL_NO_ERROR,
                          "OpenGL error - Draw Particles", __FILE__, __LINE__);
        }
      }
    }
  }
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Draw Particles",
                  __FILE__, __LINE__);
}

void GlParticleSystem::PurgeGpuResources() {
  issue_command(lib_graphics::RemoveShaderCommand(shader_id_));

  for (auto &e : emitter_data_) {
    if (!e.second.mapped_ptr) {
      glBindBuffer(GL_ARRAY_BUFFER, e.second.vbo);
      e.second.mapped_ptr = static_cast<ParticleVertex *>(glMapBufferRange(
          GL_ARRAY_BUFFER, 0, e.second.buffer_size, GL_MAP_READ_BIT));
    }

    auto max_particles = e.second.buffer_size / sizeof(ParticleVertex);
    for (int i = 0; i < e.second.particle_count; ++i) {
      auto particle_ind = ((e.second.head + i) * 4) % max_particles;
      for (int ii = 0; ii < 4; ++ii)
        e.second.saved_particles.push_back(
            e.second.mapped_ptr[particle_ind + ii]);
    }

    get_uniform_locations_ = true;
    if (e.second.sync_obj) {
      e.second.sync_obj = nullptr;
      glDeleteSync(static_cast<GLsync>(e.second.sync_obj));
    }
    glBindBuffer(GL_ARRAY_BUFFER, e.second.vbo);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    e.second.mapped_ptr = nullptr;

    glDeleteVertexArrays(1, &e.second.vao);
    glDeleteBuffers(1, &e.second.vbo);
    glDeleteBuffers(1, &e.second.ebo);
  }
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Draw Particles",
                  __FILE__, __LINE__);
}

void GlParticleSystem::RebuildGpuResources() {
  auto shader_cmd = lib_graphics::AddShaderCommand(vert_shader_, frag_shader_);
  shader_id_ = shader_cmd.ShaderId();
  issue_command(shader_cmd);
  particle_material_.shader = shader_id_;

  for (auto &e : emitter_data_) {
    auto emitter = g_ent_mgr.GetOldCbeR<ParticleEmitter>(e.first);
    AddParticleEmitter(e.first, *emitter);

    bool unmap_buffer = false;
    if (!e.second.mapped_ptr) {
      unmap_buffer = true;
      glBindBuffer(GL_ARRAY_BUFFER, e.second.vbo);
      e.second.mapped_ptr = static_cast<ParticleVertex *>(glMapBufferRange(
          GL_ARRAY_BUFFER, 0, e.second.buffer_size, GL_MAP_WRITE_BIT));
      cu::AssertError(glGetError() == GL_NO_ERROR,
                      "OpenGL error - Draw Particles Rebuild", __FILE__,
                      __LINE__);
    }

    auto max_particles = e.second.buffer_size / sizeof(ParticleVertex);
    for (int i = 0; i < e.second.particle_count; ++i) {
      auto particle_ind = ((e.second.head + i) * 4) % max_particles;
      for (int ii = 0; ii < 4; ++ii)
        e.second.mapped_ptr[particle_ind + ii] =
            e.second.saved_particles[i * 4 + ii];
    }
    e.second.saved_particles.clear();

    if (unmap_buffer) {
      glBindBuffer(GL_ARRAY_BUFFER, e.second.vbo);
      glUnmapBuffer(GL_ARRAY_BUFFER);
      e.second.mapped_ptr = nullptr;
    }
  }
  cu::AssertError(glGetError() == GL_NO_ERROR,
                  "OpenGL error - Draw Particles Rebuild", __FILE__, __LINE__);
}

void GlParticleSystem::AddParticleEmitter(lib_core::Entity ent,
                                          const ParticleEmitter &emitter) {
  bool persist_storage = false;
  auto gpu_cap = engine_->GetWindow()->Capabilities();
  if (gpu_cap.version > 4.45) persist_storage = true;

  EmitterGpuData *emitter_data;
  auto it = emitter_data_.find(ent);
  if (it == emitter_data_.end()) {
    emitter_data = &emitter_data_[ent];
    emitter_data->buffer_size =
        emitter.max_particles * 4 * sizeof(ParticleVertex);
  } else
    emitter_data = &it->second;

  ct::dyn_array<ParticleVertex> init_particles(emitter.max_particles * 4);
  for (size_t i = 0; i < emitter.max_particles; ++i) {
    init_particles[i * 4 + 0].corners = {-1.f, -1.f};
    init_particles[i * 4 + 1].corners = {1.f, -1.f};
    init_particles[i * 4 + 2].corners = {1.f, 1.f};
    init_particles[i * 4 + 3].corners = {-1.f, 1.f};
  }

  glGenVertexArrays(1, &emitter_data->vao);
  glGenBuffers(1, &emitter_data->vbo);
  glGenBuffers(1, &emitter_data->ebo);

  glBindVertexArray(emitter_data->vao);
  glBindBuffer(GL_ARRAY_BUFFER, emitter_data->vbo);

  if (persist_storage) {
    GLbitfield map_flags =
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    GLbitfield create_flags = map_flags | GL_DYNAMIC_STORAGE_BIT;

    glBufferStorage(GL_ARRAY_BUFFER, emitter_data->buffer_size,
                    init_particles.data(), create_flags);
    emitter_data->mapped_ptr = static_cast<ParticleVertex *>(glMapBufferRange(
        GL_ARRAY_BUFFER, 0, emitter_data->buffer_size, map_flags));
  } else {
    glBufferData(GL_ARRAY_BUFFER, emitter_data->buffer_size,
                 init_particles.data(), GL_DYNAMIC_DRAW);
  }
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Add Particles",
                  __FILE__, __LINE__);

  ct::dyn_array<GLuint> indices(emitter.max_particles * 6);
  for (int i = 0; i < emitter.max_particles; i++) {
    indices[i * 6 + 0] = i * 4 + 2;
    indices[i * 6 + 1] = i * 4 + 1;
    indices[i * 6 + 2] = i * 4 + 0;

    indices[i * 6 + 3] = i * 4 + 3;
    indices[i * 6 + 4] = i * 4 + 2;
    indices[i * 6 + 5] = i * 4 + 0;
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, emitter_data->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               emitter.max_particles * 6 * sizeof(GLuint), indices.data(),
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                        (GLvoid *)nullptr);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                        (GLvoid *)offsetof(ParticleVertex, position));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                        (GLvoid *)offsetof(ParticleVertex, velocity));
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                        (GLvoid *)offsetof(ParticleVertex, corners));
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                        (GLvoid *)offsetof(ParticleVertex, start_time));

  glBindVertexArray(0);
  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error - Add Particles",
                  __FILE__, __LINE__);
}

void GlParticleSystem::AddParticlePoint(ParticleVertex *buffer,
                                        const ParticleEmitter *emitter,
                                        const Transform *transform) {
  std::uniform_real_distribution<float> rng_dir(-1.f, 1.f);
  lib_core::Vector3 dir_velocity = {rng_dir(rng_engine_), rng_dir(rng_engine_),
                                    rng_dir(rng_engine_)};
  dir_velocity.Normalize();
  dir_velocity *= emitter->center_velocity;

  for (size_t ii = 0; ii < 4; ++ii) {
    buffer[ii].velocity = dir_velocity + emitter->start_velocity;
    buffer[ii].position = transform ? transform->Position() : 0.f;
  }
}

void GlParticleSystem::AddParticleCube(ParticleVertex *buffer,
                                       const ParticleEmitter *emitter,
                                       const Transform *transform) {
  std::uniform_real_distribution<float> rng_dist(-1.f, 1.f);
  lib_core::Vector3 position_modifier = {
      rng_dist(rng_engine_) * emitter->type_data[0],
      rng_dist(rng_engine_) * emitter->type_data[1],
      rng_dist(rng_engine_) * emitter->type_data[2]};

  auto center_pos = transform ? transform->Position() : 0.f;
  auto center_dir = center_pos - (center_pos + position_modifier);
  if (center_dir.Length() > emitter->center_velocity) {
    center_dir.Normalize();
    center_dir *= emitter->center_velocity;
  }

  for (size_t ii = 0; ii < 4; ++ii) {
    buffer[ii].velocity = center_dir + emitter->start_velocity;
    buffer[ii].position = center_pos + position_modifier;
  }
}

void GlParticleSystem::AddParticleSphere(ParticleVertex *buffer,
                                         const ParticleEmitter *emitter,
                                         const Transform *transform) {
  std::uniform_real_distribution<float> rng_dir(-1.f, 1.f);
  std::uniform_real_distribution<float> rng_rad(0.f, emitter->type_data[3]);
  lib_core::Vector3 random_direction = {
      rng_dir(rng_engine_) * emitter->type_data[0],
      rng_dir(rng_engine_) * emitter->type_data[1],
      rng_dir(rng_engine_) * emitter->type_data[2]};
  random_direction.Normalize();

  auto center_pos = transform ? transform->Position() : 0.f;
  auto sphere_mod = random_direction * rng_rad(rng_engine_);

  auto center_dir = center_pos - (center_pos + sphere_mod);
  if (center_dir.Length() > emitter->center_velocity) {
    center_dir.Normalize();
    center_dir *= emitter->center_velocity;
  }

  for (size_t ii = 0; ii < 4; ++ii) {
    buffer[ii].velocity = center_dir + emitter->start_velocity;
    buffer[ii].position = center_pos + sphere_mod;
  }
}

void GlParticleSystem::AddParticleCircle(ParticleVertex *buffer,
                                         const ParticleEmitter *emitter,
                                         const Transform *transform) {
  auto axis = lib_core::Vector3(emitter->type_data[0], emitter->type_data[1],
                                emitter->type_data[2]);
  axis.Normalize();

  lib_core::Quaternion rotation;
  std::uniform_real_distribution<float> rng_dist(-1.f, 1.f);
  rotation.FromAxisAngle(axis, rng_dist(rng_engine_) * PI);

  lib_core::Vector3 position_modifier = {0.f, emitter->type_data[3], 0.f};
  rotation.RotateVector(position_modifier);

  auto center_pos = transform ? transform->Position() : 0.f;
  auto center_dir = center_pos - (center_pos + position_modifier);
  if (center_dir.Length() > emitter->center_velocity) {
    center_dir.Normalize();
    center_dir *= emitter->center_velocity;
  }

  for (size_t ii = 0; ii < 4; ++ii) {
    buffer[ii].velocity = center_dir + emitter->start_velocity;
    buffer[ii].position = center_pos + position_modifier;
  }
}

void GlParticleSystem::AddParticleSquare(ParticleVertex *buffer,
                                         const ParticleEmitter *emitter,
                                         const Transform *transform) {
  auto axis = lib_core::Vector3(emitter->type_data[0], emitter->type_data[1],
                                emitter->type_data[2]);
  axis.Normalize();

  lib_core::Quaternion rotation;
  std::uniform_real_distribution<float> rng_dist(-1.f, 1.f);
  rotation.FromAxisAngle(axis, rng_dist(rng_engine_) * PI);

  lib_core::Vector3 position_modifier = {0.f, emitter->type_data[3] * 1.5f,
                                         0.f};
  rotation.RotateVector(position_modifier);

  for (int i = 0; i < 3; ++i)
    if (std::abs(position_modifier[i]) > emitter->type_data[3])
      position_modifier[i] = position_modifier[i] > 0.f
                                 ? emitter->type_data[3]
                                 : -emitter->type_data[3];

  auto center_pos = transform ? transform->Position() : 0.f;
  auto center_dir = center_pos - (center_pos + position_modifier);
  if (center_dir.Length() > emitter->center_velocity) {
    center_dir.Normalize();
    center_dir *= emitter->center_velocity;
  }

  for (size_t ii = 0; ii < 4; ++ii) {
    buffer[ii].velocity = center_dir + emitter->start_velocity;
    buffer[ii].position = center_pos + position_modifier;
  }
}
}  // namespace lib_graphics
