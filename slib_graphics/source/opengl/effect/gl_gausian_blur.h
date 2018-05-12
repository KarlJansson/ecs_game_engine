#pragma once
#include "engine_core.h"
#include "material_system.h"

namespace lib_graphics {
class GlGausianBlur {
 public:
  GlGausianBlur(lib_core::EngineCore* engine);
  ~GlGausianBlur();

  void ApplyBlurEffect(ct::dyn_array<size_t>& blur_buffer, int passes);
  void SetScreenQuad(unsigned screen_quad);

 protected:
 private:
  lib_core::EngineCore* engine_;

  unsigned screen_quad_;
  Material blur_material_;

  int buffer_id_loc_ = -1;
};
}  // namespace lib_graphics
