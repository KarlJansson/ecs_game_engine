#pragma once
#include "engine_core.h"
#include "material_system.h"

namespace lib_graphics {
class GlSmaa {
 public:
  GlSmaa(lib_core::EngineCore *engine);
  ~GlSmaa();

  void ApplySmaa();

  unsigned OutputFramebuffer();

 private:
  void ShaderInclude(std::string &shader);
  void ReplaceAll(std::string &str, const std::string &from,
                  const std::string &to);
  void CreateShader(std::string vs_text, std::string ps_text,
                    unsigned *program);

  unsigned edge_tex_;
  unsigned blend_tex_;
  unsigned area_tex_;
  unsigned search_tex_;
  unsigned albedo_tex_;

  unsigned edge_fbo_;
  unsigned blend_fbo_;
  unsigned output_fbo_;

  unsigned edge_shader_;
  unsigned blend_shader_;
  unsigned neighborhood_shader_;

  lib_core::EngineCore *engine_;
};
}  // namespace lib_graphics
