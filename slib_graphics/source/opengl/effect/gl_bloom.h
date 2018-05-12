#pragma once
#include "engine_core.h"
#include "material_system.h"

namespace lib_graphics {
class GlBloom {
 public:
  GlBloom(std::pair<size_t, size_t> dim, lib_core::EngineCore *engine,
          class GlGausianBlur *blur_effect, TextureDesc rme_gbuffer,
          TextureDesc hdr_buffer);
  ~GlBloom();

  void ApplyBloomEffect();
  void SetBloomTextureSize(std::pair<size_t, size_t> dim);
  void SetScreenQuad(unsigned quad);

  TextureDesc &GetBloomTexture();

 protected:
 private:
  std::pair<size_t, size_t> bloom_dim_;
  lib_core::EngineCore *engine_;
  class GlGausianBlur *blur_effect_;

  unsigned screen_quad_;
  size_t bloom_extract_material_;
  size_t bloom_shader_;

  int hdr_tex_loc_ = -1;
  TextureDesc bloom_return_desc_;
  ct::dyn_array<size_t> bloom_buffer_;
};
}  // namespace lib_graphics
