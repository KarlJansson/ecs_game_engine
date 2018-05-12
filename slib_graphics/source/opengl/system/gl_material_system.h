#pragma once
#include "gl_stock_shaders.h"
#include "material_system.h"

namespace lib_graphics {
class GlMaterialSystem : public MaterialSystem {
 public:
  GlMaterialSystem(lib_core::EngineCore *engine);
  ~GlMaterialSystem();

  void DrawUpdate(lib_graphics::Renderer *renderer,
                  lib_gui::TextSystem *text_renderer) override;

  bool ForceMaterial(Material &mat,
                     ct::dyn_array<std::pair<lib_core::Entity, Light>> *lights =
                         nullptr) override;
  size_t GetStockShaderId(ShaderType type) override;
  size_t GetStockShaderIdConvert(size_t id);

  void RebuildTextures() override;
  void PurgeGpuResources() override;

  bool PushFrameBuffer(size_t draw_buffer, size_t read_buffer) override;
  bool PushFrameBuffer(size_t draw_buffer) override;
  void PopFrameBuffer() override;

  void CompileShaders();
  uint32_t GetCurrentShader() override;
  unsigned GetShaderId(size_t id);

  void ForceFreeFramebuffer(size_t frame_buffer_id, bool erase = true);

  size_t Get2DShadowFrameBuffer(size_t res);
  size_t Get3DShadowFrameBuffer(size_t res);

  unsigned TextureById(size_t id);

 protected:
 private:
  ct::hash_map<size_t, size_t> shadow_frame_buffers_2d_;
  ct::hash_map<size_t, size_t> shadow_frame_buffers_3d_;

  size_t CreateCubeMapShadowTarget(size_t res);
  size_t Create2DShadowTarget(size_t res_x, size_t res_y);
  void CreateTexture2D(size_t id);
  void CreateBlurTargets(CreateBlurFrameBufferCommand &command);
  void CreateDeferredRendererFrameBuffer(
      CreateDeferedFrameBufferCommand &command);
  void CreateHdrFrameBuffer(CreateHdrFrameBufferCommand &command);
  void Create2DTextureTarget(CreateTextureFrameBufferCommand &fb_command);
  void CreateFrameBuffer(size_t fb_id, size_t dim_x, size_t dim_y);
  void CompileShaderRecource(AddShaderCommand &code);
  void LoadTexture2D(Texture &texture, bool force = false);
  void LoadTexture3D(ct::dyn_array<Texture *> &faces, size_t name_hash,
                     bool force = false);

  unsigned current_shader_;
  GlStockShaders stock_shaders_;

  ct::dyn_array<size_t> rem_fb_vec;
  size_t clean_counter_ = 0;

  ct::hash_set<size_t> removed_fbs_;

  ct::hash_set<size_t> collection_exempt_;
  ct::hash_set<size_t> used_textures_;
  ct::hash_set<size_t> suspended_textures_;

  ct::hash_map<size_t, unsigned> shaders_;
  ct::hash_map<size_t, std::pair<size_t, ct::dyn_array<Texture *>>> texture_3d_;
  ct::hash_map<size_t, std::pair<unsigned, TextureType>> textures_;

  ct::dyn_array<size_t> mipmap_generation_queue_;
  ct::tree_set<size_t> shadow_buffer_use_2d_;
  ct::tree_set<size_t> shadow_buffer_use_3d_;
};
}  // namespace lib_graphics
