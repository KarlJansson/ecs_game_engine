#pragma once
#include <thread>
#include "entity.h"
#include "graphics_commands.h"
#include "system.h"
#include "vector_def.h"

namespace lib_gui {
class TextSystem;
}

namespace lib_core {
class EngineCore;
}

namespace lib_graphics {
class MaterialSystem : public lib_core::System {
 protected:
  enum TextureType { kTexture2D, kTexture3D };

  struct Texture {
    size_t pack_hash;
    size_t name_hash;

    size_t start_point;

    short nr_channels;
    std::pair<size_t, size_t> dim;
    ct::dyn_array<uint8_t> data;
    bool loaded = false;
  };

 public:
  struct FrameBuffer {
    bool purged = false;

    uint32_t fbo;
    std::pair<size_t, size_t> dim;
    ct::dyn_array<TextureDesc> textures;
    ct::dyn_array<uint32_t> rend_buffers;

    RenderToTextureCommand last_command;
  };

  enum ShaderType {
    kText,

    kPbrUntextured,
    kPbrTextured,
    kDeferredLightingAmbient,

    kTonemapGamma
  };

  MaterialSystem(lib_core::EngineCore *engine);
  ~MaterialSystem() override;

  void StartLoadThread();
  void TerminateLoadThread();

  Material CreateTexturedMaterial(const ct::string &alb, const ct::string &norm,
                                  const ct::string &rme);
  Material CreateUntexturedMaterial();

  size_t AddTexture2D(const ct::string &texture, bool blocking = false);
  size_t AddTexture3D(ct::dyn_array<ct::string> &texture);
  ct::dyn_array<std::pair<size_t, ct::string>> LoadTexturePack(
      const ct::string &pack_path);
  size_t GetTextureId(size_t tex_hash);
  size_t GetTextureId(const ct::string &tex_name);

  void ApplyMaterial(
      size_t mat_id,
      ct::dyn_array<std::pair<lib_core::Entity, class Light>> *lights = nullptr,
      bool force = false);

  Material GetMaterial(size_t mat_id);
  FrameBuffer *GetFrameBuffer(size_t fb_id);

  virtual bool PushFrameBuffer(size_t draw_buffer, size_t read_buffer) = 0;
  virtual bool PushFrameBuffer(size_t draw_buffer) = 0;
  virtual void PopFrameBuffer() = 0;

  virtual void PurgeGpuResources() = 0;
  virtual void RebuildTextures() = 0;
  virtual size_t GetStockShaderId(ShaderType type) = 0;
  virtual bool ForceMaterial(
      Material &mat,
      ct::dyn_array<std::pair<lib_core::Entity, class Light>> *lights =
          nullptr) = 0;

  size_t GetCurrentMaterial();
  virtual uint32_t GetCurrentShader() = 0;

 protected:
  lib_core::EngineCore *engine_;

  size_t current_material_ = 0;
  size_t current_shader_id_ = 0;

  ct::hash_map<size_t, Material> materials_;
  ct::hash_map<size_t, std::unique_ptr<Texture>> texture_source_;

  tbb::concurrent_queue<size_t> add_textures_;
  tbb::concurrent_queue<ct::dyn_array<size_t>> add_texture_3d_;

  tbb::concurrent_unordered_map<size_t, size_t> texture_map_;
  ct::hash_map<size_t, ct::dyn_array<std::pair<size_t, ct::string>>>
      loaded_tex_packs_;
  tbb::concurrent_unordered_map<size_t, ct::string> name_hashes_;

  ct::dyn_array<size_t> available_framebuffers_;
  ct::hash_map<size_t, FrameBuffer> frame_buffers_;
  ct::dyn_array<std::pair<size_t, size_t>> frame_buffer_stack_;

 private:
  bool run_tex_load_thread_;
  std::condition_variable tex_load_cond_;

  struct TextureLoadJob {
    ct::dyn_array<size_t> pack_hash;
    ct::dyn_array<size_t> tex_id;
    ct::dyn_array<size_t> start_point;
    ct::dyn_array<ct::dyn_array<uint8_t> *> data;
  };

  tbb::concurrent_queue<TextureLoadJob> tex_load_jobs_;
  void TextureLoaderThread();

  std::unique_ptr<std::thread> texture_unpack_thread_;
};
}  // namespace lib_graphics
