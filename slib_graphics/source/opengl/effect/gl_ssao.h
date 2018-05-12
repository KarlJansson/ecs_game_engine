#pragma once
#include "camera.h"
#include "engine_core.h"
#include "graphics_commands.h"

namespace lib_graphics {
class GlSsao {
 public:
  GlSsao(std::pair<size_t, size_t> dim, class GlGausianBlur *blur,
         lib_core::EngineCore *engine, TextureDesc pos_gbuffer,
         TextureDesc normal_gbuffer);
  ~GlSsao();

  void ApplySsaoEffect(Camera &cam);
  void SetScreenQuad(unsigned quad);

  TextureDesc &GetSsaoTexture();

 private:
  void CheckShaderLocations();

  std::pair<size_t, size_t> ssao_dim_;
  class GlGausianBlur *blur_effect_;
  lib_core::EngineCore *engine_;

  int kernel_smaples_ = 32;
  float radius_ = 0.5f;
  float bias_ = 0.005f;

  TextureDesc ssao_return_tex_;

  unsigned screen_quad_;
  size_t ssao_material_;
  size_t ssao_shader_;

  unsigned noise_texture_;
  ct::dyn_array<float> ssao_kernel_;

  ct::dyn_array<size_t> ssao_buffer_;

  int view_loc_ = -1;
  int proj_loc_ = -1;
  int ksize_loc_ = -1;
  int radius_loc_ = -1;
  int nscale_loc_ = -1;
  int bias_loc_ = -1;
  int samples_loc_ = -1;
  int noise_loc_ = -1;
};
}  // namespace lib_graphics
