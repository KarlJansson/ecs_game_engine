#include "mesh_system.h"
#include <fstream>
#include <utility>
#include "../../source_shared/include/serialization_utilities.hpp"
#include "axis_aligned_box.h"
#include "core_utilities.h"
#include "culling_system.h"
#include "entity_manager.h"
#include "graphics_commands.h"
#include "mesh.h"
#include "physics_commands.h"
#include "physics_system.h"
#include "system_manager.h"

namespace lib_graphics {
MeshSystem::MeshSystem(const lib_core::EngineCore* engine) : engine_(engine) {}

MeshSystem::~MeshSystem() { TerminateLoadThread(); }

void MeshSystem::LogicUpdate(float dt) {
  auto new_meshes = g_ent_mgr.GetNewCbt<Mesh>();
  if (new_meshes) {
    auto old_meshes = g_ent_mgr.GetOldCbt<Mesh>();
    auto mesh_update = g_ent_mgr.GetNewUbt<Mesh>();

    auto mesh_thread = [&](tbb::blocked_range<size_t>& range) {
      for (size_t i = range.begin(); i != range.end(); ++i) {
        if (!(*mesh_update)[i]) continue;

        auto& new_mesh = new_meshes->at(i);
        auto& old_mesh = old_meshes->at(i);

        new_mesh.albedo = old_mesh.albedo;
        new_mesh.rme = old_mesh.rme;
        new_mesh.texture_scale = old_mesh.texture_scale;
        new_mesh.texture_offset = old_mesh.texture_offset;
        if (new_mesh.material != old_mesh.material)
          new_mesh.material = old_mesh.material;
        if (new_mesh.mesh != old_mesh.mesh) new_mesh.mesh = old_mesh.mesh;

        if (new_mesh.fade_in < new_mesh.translucency) {
          new_mesh.fade_in += dt;
          if (new_mesh.fade_in > new_mesh.translucency)
            new_mesh.fade_in = new_mesh.translucency;
        }

        (*mesh_update)[i] = false;
      }
    };

    tbb::parallel_for(tbb::blocked_range<size_t>(0, mesh_update->size()),
                      mesh_thread);
  }
}

ct::dyn_array<size_t> MeshSystem::LoadModelPack(const ct::string& path) {
  ct::dyn_array<size_t> return_array;
  auto path_hash = std::hash<ct::string>{}(path);
  auto pack_it = loaded_model_packs_.find(path_hash);
  if (pack_it != loaded_model_packs_.end()) return return_array;

  model_pack_name_map_[path_hash] = path;
  std::ifstream open(path, std::ios::binary);

  size_t model_count;
  ct::dyn_array<size_t> read_order;
  ct::hash_map<size_t, Model> models;

  open.read((char*)&model_count, sizeof(model_count));
  size_t name_length;
  ct::string model_name;
  size_t model_hash;
  for (int i = 0; i < model_count; ++i) {
    Model model;
    open.read((char*)&name_length, sizeof(name_length));
    model_name.clear();
    model_name.assign(name_length, ' ');
    open.read((char*)model_name.data(), sizeof(uint8_t) * name_length);

    if (model_name.find('\\') != ct::string::npos)
      model_name = model_name.substr(model_name.find_last_of('\\') + 1,
                                     model_name.size());
    if (model_name.find('/') != ct::string::npos)
      model_name = model_name.substr(model_name.find_last_of('/') + 1,
                                     model_name.size());

    open.read((char*)&model.nr_meshes, sizeof(model.nr_meshes));
    open.read((char*)&model.start, sizeof(model.start));

    model_hash = std::hash<ct::string>{}(model_name);
    model_pack_map_[model_hash] = path_hash;
    models[model_hash] = std::move(model);
    read_order.push_back(model_hash);
  }

  for (auto& ind : read_order) {
    auto& model = models[ind];
    for (int i = 0; i < model.nr_meshes; ++i) {
      model.meshes.push_back(g_sys_mgr.GenerateResourceIds(1));
      return_array.push_back(model.meshes.back());
    }

    model_load_jobs_.push({path_hash, model.meshes, model.start});
  }

  load_cond_.notify_all();
  loaded_model_packs_[path_hash] = std::move(models);
  return return_array;
}

size_t MeshSystem::GetModelHash(const ct::string& name) const {
  return std::hash<ct::string>{}(name);
}

const MeshSystem::Model* MeshSystem::GetModel(size_t model_hash) const {
  auto it = model_pack_map_.find(model_hash);
  if (it == model_pack_map_.end()) return nullptr;
  auto it_2 = loaded_model_packs_.find(it->second);
  if (it_2 == loaded_model_packs_.end()) return nullptr;
  auto it_3 = it_2->second.find(model_hash);
  if (it_3 == it_2->second.end()) return nullptr;
  return &it_3->second;
}

const MeshSystem::Model* MeshSystem::GetModel(const ct::string& name) const {
  return GetModel(GetModelHash(name));
}

const MeshInit* MeshSystem::GetMeshSource(size_t mesh_id) const {
  auto it = mesh_source_.find(mesh_id);
  if (it == mesh_source_.end()) return nullptr;
  return &it->second;
}

void MeshSystem::StartLoadThread() {
  model_unpack_thread_ =
      std::make_unique<std::thread>([&]() { this->ModelLoaderThread(); });
}

void MeshSystem::TerminateLoadThread() {
  load_models_ = false;
  load_cond_.notify_all();
  model_unpack_thread_->join();
}

void MeshSystem::ModelLoaderThread() {
  ModelLoadJob job;
  ct::hash_map<size_t, std::ifstream> files;
  ct::hash_map<size_t, std::ifstream>::iterator file_it;
  ct::dyn_array<uint8_t> compressed, decompressed;
  size_t data_size;

#ifdef UnixBuild
  std::mutex mtx;
#endif
  while (load_models_) {
    while (model_load_jobs_.try_pop(job)) {
      file_it = files.find(job.pack_hash);
      if (file_it == files.end()) {
        auto name_it = model_pack_name_map_.find(job.pack_hash);
        files[job.pack_hash] = std::ifstream(name_it->second, std::ios::binary);
        file_it = files.find(job.pack_hash);
      }

      file_it->second.seekg(job.start_point);
      file_it->second.read((char*)&data_size, sizeof(data_size));
      compressed.assign(data_size, 0);
      file_it->second.read((char*)compressed.data(), data_size);

      cu::DecompressMemory(compressed, decompressed);
      compressed.clear();

      auto it = decompressed.begin();
      for (unsigned long mesh_id : job.mesh_ids) {
        MeshInit mesh_init;
        SerializationUtilities::ReadFromBuffer(it, mesh_init.center);
        SerializationUtilities::ReadFromBuffer(it, mesh_init.extent);
        SerializationUtilities::ReadFromBuffer(it, mesh_init.vertices);
        SerializationUtilities::ReadFromBuffer(it, mesh_init.indices);

        issue_command(AddModelMeshCommand(mesh_id, mesh_init));
      }
      decompressed.clear();
    }
#ifdef UnixBuild
    auto lock = std::unique_lock<std::mutex>(mtx, std::defer_lock);
    load_cond_.wait(lock);
#elif WindowsBuild
    load_cond_.wait(std::unique_lock<std::mutex>(std::mutex()));
#endif
  }
}
}  // namespace lib_graphics
