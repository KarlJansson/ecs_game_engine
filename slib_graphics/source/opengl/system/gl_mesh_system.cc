#include "gl_mesh_system.h"
#include <GL/glew.h>
#include "culling_system.h"
#include "entity_manager.h"
#include "graphics_commands.h"
#include "mesh.h"
#include "physics_commands.h"
#include "physics_system.h"

namespace lib_graphics {
GlMeshSystem::GlMeshSystem(lib_core::EngineCore *engine) : MeshSystem(engine) {}

GlMeshSystem::~GlMeshSystem() {
  GLsizei size = GLsizei(meshes_.size());
  ct::dyn_array<GLuint> vaos, vbos, ebos;

  for (auto &mesh : meshes_) {
    vaos.push_back(mesh.second.vao);
    vbos.push_back(mesh.second.vbo);
    ebos.push_back(mesh.second.ebo);
  }

  if (!meshes_.empty()) {
    glDeleteVertexArrays(size, vaos.data());
    glDeleteBuffers(size, vbos.data());
    glDeleteBuffers(size, ebos.data());
  }
}

void GlMeshSystem::DrawMesh(size_t mesh_id, int amount, bool force) {
  auto it = meshes_.find(mesh_id);
  if (it == meshes_.end()) return;

  if (current_mesh_ != mesh_id || force) glBindVertexArray(it->second.vao);
  glDrawElementsInstanced(GL_TRIANGLES, it->second.ind_count, GL_UNSIGNED_INT,
                          nullptr, amount);
  current_mesh_ = mesh_id;
  ++draw_calls_;
}

void GlMeshSystem::DrawUpdate(lib_graphics::Renderer *renderer,
                              lib_gui::TextSystem *text_renderer) {
  current_mesh_ = -1;

  if (engine_ && engine_->GetDebugOutput())
    engine_->GetDebugOutput()->UpdateBottomLeftLine(
        6, "Total Mesh Draw Calls: " + std::to_string(draw_calls_));
  draw_calls_ = 0;

  auto add_mesh_func = [&](size_t id, MeshInit &source) {
    MeshInfo info;
    glGenVertexArrays(1, &info.vao);
    glGenBuffers(1, &info.vbo);
    glGenBuffers(1, &info.ebo);

    glBindVertexArray(info.vao);
    glBindBuffer(GL_ARRAY_BUFFER, info.vbo);

    glBufferData(GL_ARRAY_BUFFER, source.vertices.size() * sizeof(Vertex),
                 &source.vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, info.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 source.indices.size() * sizeof(GLuint), &source.indices[0],
                 GL_STATIC_DRAW);

    // Vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (GLvoid *)0);

    // Vertex Normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (GLvoid *)offsetof(Vertex, normal));

    // Vertex Tangents
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (GLvoid *)offsetof(Vertex, tangent));

    // Vertex Texture Coords
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (GLvoid *)offsetof(Vertex, texcoord));

    glBindVertexArray(0);

    info.ind_count = GLsizei(source.indices.size());
    meshes_[id] = info;
  };

  auto add_mesh_commands = g_sys_mgr.GetCommands<AddMeshCommand>();
  if (add_mesh_commands && !add_mesh_commands->empty()) {
    for (auto &c : *add_mesh_commands) {
      BoundingVolume aabb;
      lib_physics::PhysicsInit pinit;
      aabb.center = c.mesh_init.center;
      aabb.extent = c.mesh_init.extent;

      pinit.inds = c.mesh_init.indices;
      pinit.verts.reserve(c.mesh_init.vertices.size());
      for (auto &vert : c.mesh_init.vertices)
        pinit.verts.push_back({vert.position});

      issue_command(lib_physics::AddMeshSourceCommmand(c.MeshId(), pinit));
      issue_command(CullingSystem::AddMeshAabbCommand(c.MeshId(), aabb));

      add_mesh_func(c.MeshId(), c.mesh_init);
      mesh_source_[c.MeshId()] = std::move(c.mesh_init);
    }
    add_mesh_commands->clear();
  }

  auto add_model_mesh_commands = g_sys_mgr.GetCommands<AddModelMeshCommand>();
  if (add_model_mesh_commands && !add_model_mesh_commands->empty()) {
    for (auto &c : *add_model_mesh_commands) {
      BoundingVolume aabb;
      lib_physics::PhysicsInit pinit;
      aabb.center = c.mesh_init.center;
      aabb.extent = c.mesh_init.extent;

      pinit.inds = c.mesh_init.indices;
      pinit.verts.reserve(c.mesh_init.vertices.size());
      for (auto &vert : c.mesh_init.vertices)
        pinit.verts.push_back({vert.position});

      issue_command(lib_physics::AddMeshSourceCommmand(c.MeshId(), pinit));
      issue_command(CullingSystem::AddMeshAabbCommand(c.MeshId(), aabb));

      add_mesh_func(c.MeshId(), c.mesh_init);
      mesh_source_[c.MeshId()] = std::move(c.mesh_init);
    }
    add_model_mesh_commands->clear();
  }

  auto remove_mesh_commands = g_sys_mgr.GetCommands<RemoveMeshCommand>();
  if (remove_mesh_commands && !remove_mesh_commands->empty()) {
    for (auto &c : *remove_mesh_commands) {
      auto it = meshes_.find(c.mesh_id);
      if (it == meshes_.end()) continue;

      glDeleteBuffers(1, &it->second.vbo);
      glDeleteBuffers(1, &it->second.ebo);
      glDeleteVertexArrays(1, &it->second.vao);

      issue_command(CullingSystem::RemoveMeshAabbCommand(c.mesh_id));
      issue_command(lib_physics::RemoveMeshSourceCommmand(c.mesh_id));
      meshes_.erase(it);
    }
    remove_mesh_commands->clear();
  }

  cu::AssertError(glGetError() == GL_NO_ERROR, "OpenGL error.", __FILE__,
                  __LINE__);
}

void GlMeshSystem::RebuildResources() {
  for (auto &mesh : meshes_) {
    auto &mesh_init = mesh_source_[mesh.first];
    MeshInfo info;
    glGenVertexArrays(1, &info.vao);
    glGenBuffers(1, &info.vbo);
    glGenBuffers(1, &info.ebo);

    glBindVertexArray(info.vao);
    glBindBuffer(GL_ARRAY_BUFFER, info.vbo);

    glBufferData(GL_ARRAY_BUFFER, mesh_init.vertices.size() * sizeof(Vertex),
                 &mesh_init.vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, info.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 mesh_init.indices.size() * sizeof(uint32_t),
                 &mesh_init.indices[0], GL_STATIC_DRAW);

    // Vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (GLvoid *)0);

    // Vertex Normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (GLvoid *)offsetof(Vertex, normal));

    // Vertex Tangents
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (GLvoid *)offsetof(Vertex, tangent));

    // Vertex Texture Coords
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (GLvoid *)offsetof(Vertex, texcoord));

    glBindVertexArray(0);

    info.ind_count = GLsizei(mesh_init.indices.size());
    meshes_[mesh.first] = info;
  }
}

void GlMeshSystem::PurgeGpuResources() {
  for (auto &mesh : meshes_) {
    glDeleteBuffers(1, &mesh.second.vbo);
    glDeleteBuffers(1, &mesh.second.ebo);
    glDeleteVertexArrays(1, &mesh.second.vao);
  }
}
}  // namespace lib_graphics
