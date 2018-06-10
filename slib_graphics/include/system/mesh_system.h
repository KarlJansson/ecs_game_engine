#pragma once
#include <thread>
#include "engine_core.h"
#include "graphics_commands.h"
#include "system.h"
#include "vector_def.h"
#include "vertex.h"

namespace lib_graphics {
class MeshSystem : public lib_core::System {
 public:
  MeshSystem(const lib_core::EngineCore* engine);
  ~MeshSystem() override;

  struct Model {
    size_t start;
    size_t nr_meshes;
    ct::dyn_array<size_t> meshes;
  };

  void LogicUpdate(float dt) override;

  virtual void PurgeGpuResources() = 0;
  virtual void RebuildResources() = 0;
  virtual void DrawMesh(size_t mesh_id, int amount = 1, bool force = false) = 0;

  ct::dyn_array<size_t> LoadModelPack(const ct::string& path);

  size_t GetModelHash(const ct::string& name) const;
  const Model* GetModel(size_t model_hash) const;
  const Model* GetModel(ct::string name) const;
  const MeshInit* GetMeshSource(size_t mesh_id) const;

  void StartLoadThread();
  void TerminateLoadThread();

 protected:
  const lib_core::EngineCore* engine_;
  ct::hash_map<size_t, size_t> model_pack_map_;
  ct::hash_map<size_t, MeshInit> mesh_source_;
  ct::hash_map<size_t, ct::hash_map<size_t, Model>> loaded_model_packs_;

 private:
  struct ModelLoadJob {
    size_t pack_hash;
    ct::dyn_array<size_t> mesh_ids;
    size_t start_point;
  };

  std::condition_variable load_cond_;
  bool load_models_ = true;
  std::condition_variable model_load_cond_;

  tbb::concurrent_queue<ModelLoadJob> model_load_jobs_;
  std::unique_ptr<std::thread> model_unpack_thread_;

  void ModelLoaderThread();

  tbb::concurrent_unordered_map<size_t, ct::string> model_pack_name_map_;
};
}  // namespace lib_graphics
