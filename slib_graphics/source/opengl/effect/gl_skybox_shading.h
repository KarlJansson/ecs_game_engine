#pragma once
#include "camera.h"
#include "engine_core.h"
#include "entity_manager.h"
#include "graphics_commands.h"

namespace lib_graphics {
class GlSkyboxShading {
 public:
  GlSkyboxShading(lib_core::EngineCore* engine);
  ~GlSkyboxShading();

  void DrawSkybox(Camera& cam, lib_core::Entity cam_ent);

 protected:
 private:
  lib_core::EngineCore* engine_;

  int view_loc_ = -1;
  int proj_loc_ = -1;
  Material skybox_material_;
};
}  // namespace lib_graphics
