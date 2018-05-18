#include "material_system.h"
#include <fstream>
#include "core_utilities.h"
#include "engine_core.h"
#include "entity_manager.h"
#include "light.h"
#include "window.h"

namespace lib_graphics {
size_t MaterialSystem::AddTexture2D(ct::string texture, bool blocking) {
  auto name_hash = std::hash<ct::string>{}(texture);
  auto it = texture_map_.find(name_hash);
  if (it == texture_map_.end()) return 0;
  auto s_it = texture_source_.find(it->second);
  if (s_it == texture_source_.end()) return 0;

  auto tex_id = GetTextureId(name_hash);
  name_hashes_[name_hash] = texture;

  if (!s_it->second->loaded) {
    TextureLoadJob load_job;
    load_job.tex_id.push_back(tex_id);
    load_job.pack_hash.push_back(s_it->second->pack_hash);
    load_job.start_point.push_back(s_it->second->start_point);
    load_job.data.push_back(&s_it->second->data);
    s_it->second->loaded = true;

    tex_load_jobs_.push(load_job);
    tex_load_cond_.notify_one();
  }

  return tex_id;
}

size_t MaterialSystem::AddTexture3D(ct::dyn_array<ct::string> &texture) {
  ct::string name = "";
  TextureLoadJob load_job;
  for (auto &f : texture) {
    auto tex = GetTextureId(std::hash<ct::string>{}(f));
    auto s_it = texture_source_.find(tex);

    if (!s_it->second->loaded) {
      load_job.pack_hash.push_back(s_it->second->pack_hash);
      load_job.data.push_back(&s_it->second->data);
      load_job.start_point.push_back(s_it->second->start_point);
      s_it->second->loaded = true;
    }

    load_job.tex_id.emplace_back(tex);
    name += f;
  }

  auto name_hash = std::hash<ct::string>{}(name);
  name_hashes_[name_hash] = name;
  auto tex_id = GetTextureId(name_hash);

  if (!load_job.data.empty()) {
    tex_load_jobs_.push(load_job);
    tex_load_cond_.notify_one();
  }

  return tex_id;
}

ct::dyn_array<std::pair<size_t, ct::string>> MaterialSystem::LoadTexturePack(
    ct::string pack_path) {
  auto name_hash = std::hash<ct::string>{}(pack_path);
  auto tex_it = loaded_tex_packs_.find(name_hash);
  if (tex_it != loaded_tex_packs_.end()) return tex_it->second;

  Texture tex;
  size_t size;
  size_t nr_textures;
  ct::dyn_array<std::pair<size_t, ct::string>> tex_names;

  std::ifstream texpack_file(pack_path, std::ios::binary);
  if (texpack_file.fail()) return tex_names;

  texpack_file.read((char *)&nr_textures, sizeof(size_t));
  if (nr_textures == 0) return tex_names;

  tex.pack_hash = name_hash;
  for (int i = 0; i < nr_textures; ++i) {
    texpack_file.read((char *)&size, sizeof(size));
    ct::string tex_name;
    tex_name.assign(size, ' ');
    texpack_file.read((char *)tex_name.data(), sizeof(uint8_t) * size);
    if (tex_name.find('\\') != ct::string::npos)
      tex_name =
          tex_name.substr(tex_name.find_last_of('\\') + 1, tex_name.size());
    if (tex_name.find('/') != ct::string::npos)
      tex_name =
          tex_name.substr(tex_name.find_last_of('/') + 1, tex_name.size());

    short channels;
    texpack_file.read((char *)&channels, sizeof(channels));
    tex.nr_channels = channels;

    std::pair<int, int> dim_tmp;
    texpack_file.read((char *)&dim_tmp, sizeof(dim_tmp));
    tex.dim.first = dim_tmp.first;
    tex.dim.second = dim_tmp.second;

    texpack_file.read((char *)&tex.start_point, sizeof(tex.start_point));

    size_t id;
    auto tex_hash = std::hash<ct::string>{}(tex_name);
    tex.name_hash = tex_hash;

    auto it = texture_map_.find(tex_hash);
    if (it != texture_map_.end())
      id = it->second;
    else {
      id = g_sys_mgr.GenerateResourceIds(1);
      texture_map_[tex_hash] = id;
    }
    name_hashes_[tex_hash] = tex_name;
    texture_source_[id] = std::make_unique<Texture>(tex);
    tex_names.push_back({id, tex_name});
  }

  name_hashes_[name_hash] = pack_path;
  loaded_tex_packs_[name_hash] = tex_names;
  return tex_names;
}

void MaterialSystem::ApplyMaterial(
    size_t mat_id, ct::dyn_array<std::pair<lib_core::Entity, Light>> *lights,
    bool force) {
  if (current_material_ == mat_id && !force) return;
  auto it = materials_.find(mat_id);
  if (it == materials_.end()) return;

  if (ForceMaterial(it->second, lights)) current_material_ = mat_id;
}

Material MaterialSystem::GetMaterial(size_t mat_id) {
  auto it = materials_.find(mat_id);
  if (it == materials_.end()) return Material();
  return it->second;
}

MaterialSystem::FrameBuffer *MaterialSystem::GetFrameBuffer(size_t fb_id) {
  auto it = frame_buffers_.find(fb_id);
  if (it != frame_buffers_.end()) {
    return &it->second;
  }
  return nullptr;
}

size_t MaterialSystem::GetCurrentMaterial() { return current_material_; }

void MaterialSystem::TextureLoaderThread() {
  int i;
  TextureLoadJob job;
  ct::hash_map<size_t, std::ifstream> files;
  ct::hash_map<size_t, std::ifstream>::iterator file_it;

#ifdef UnixBuild
  std::mutex mtx;
#endif
  while (run_tex_load_thread_) {
#ifdef UnixBuild
    auto lock = std::unique_lock<std::mutex>(mtx, std::defer_lock);
    tex_load_cond_.wait(lock);
#elif WindowsBuild
    tex_load_cond_.wait(std::unique_lock<std::mutex>(std::mutex()));
#endif

    engine_->GetWindow()->SetLoadContext();

    while (tex_load_jobs_.try_pop(job)) {
      for (i = 0; i < job.data.size(); ++i) {
        file_it = files.find(job.pack_hash[i]);
        if (file_it == files.end()) {
          auto name_it = name_hashes_.find(job.pack_hash[i]);
          files[job.pack_hash[i]] =
              std::ifstream(name_it->second, std::ios::binary);
          file_it = files.find(job.pack_hash[i]);
        }

        file_it->second.seekg(job.start_point[i]);

        size_t data_size;
        file_it->second.read((char *)&data_size, sizeof(data_size));

        ct::dyn_array<uint8_t> compressed_tex(data_size);
        file_it->second.read((char *)compressed_tex.data(), data_size);

        cu::DecompressMemory(compressed_tex, *job.data[i]);

        if (job.tex_id.size() == 1) add_textures_.push(job.tex_id[i]);
      }

      if (job.tex_id.size() > 1) add_texture_3d_.push(job.tex_id);
    }
  }
}

size_t MaterialSystem::GetTextureId(size_t tex_hash) {
  size_t id;
  auto it = texture_map_.find(tex_hash);
  if (it != texture_map_.end())
    id = it->second;
  else {
    id = g_sys_mgr.GenerateResourceIds(1);
    texture_map_[tex_hash] = id;
  }
  return id;
}

size_t MaterialSystem::GetTextureId(ct::string tex_name) {
  return GetTextureId(std::hash<ct::string>{}(tex_name));
}

MaterialSystem::MaterialSystem(lib_core::EngineCore *engine)
    : engine_(engine) {}

void MaterialSystem::StartLoadThread() {
  run_tex_load_thread_ = true;
  texture_unpack_thread_ =
      std::make_unique<std::thread>([&]() { this->TextureLoaderThread(); });
}

void MaterialSystem::TerminateLoadThread() {
  run_tex_load_thread_ = false;
  tex_load_cond_.notify_all();
  texture_unpack_thread_->join();
}

MaterialSystem::~MaterialSystem() { TerminateLoadThread(); }

Material MaterialSystem::CreateTexturedMaterial(ct::string alb, ct::string norm,
                                                ct::string rme) {
  Material mat;
  mat.shader = GetStockShaderId(lib_graphics::MaterialSystem::kPbrTextured);
  mat.textures.push_back({AddTexture2D(alb), "albedo_tex"});
  mat.textures.push_back({AddTexture2D(norm), "normal_tex"});
  mat.textures.push_back({AddTexture2D(rme), "rma_tex"});
  return mat;
}

Material MaterialSystem::CreateUntexturedMaterial() {
  Material mat;
  mat.shader = GetStockShaderId(lib_graphics::MaterialSystem::kPbrUntextured);
  return mat;
}
}  // namespace lib_graphics
