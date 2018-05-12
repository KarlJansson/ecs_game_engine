#pragma once
#include "axis_aligned_box.h"
#include "mesh_system.h"
#include "vertex.h"

namespace lib_graphics {
class GlMeshSystem : public MeshSystem {
 public:
  GlMeshSystem(lib_core::EngineCore* engine);
  ~GlMeshSystem();

  void DrawMesh(size_t mesh_id, int amount = 1, bool force = false) override;

  void DrawUpdate(lib_graphics::Renderer* renderer,
                  lib_gui::TextSystem* text_renderer) override;

  void RebuildResources() override;
  void PurgeGpuResources() override;

  struct MeshInfo {
    int ind_count;
    unsigned vao;
    unsigned vbo;
    unsigned ebo;
  };

 private:
  size_t current_mesh_ = 0;

  size_t draw_calls_;
  ct::hash_map<size_t, MeshInfo> meshes_;
};
}  // namespace lib_graphics
