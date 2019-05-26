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
  void InitializeShaders();

  unsigned edge_tex_{0};
  unsigned blend_tex_{0};
  unsigned area_tex_{0};
  unsigned search_tex_{0};
  unsigned albedo_tex_{0};

  unsigned edge_fbo_{0};
  unsigned blend_fbo_{0};
  unsigned output_fbo_{0};

  unsigned edge_shader_{0};
  unsigned blend_shader_{0};
  unsigned neighborhood_shader_{0};

  lib_core::EngineCore *engine_;
};
}  // namespace lib_graphics
