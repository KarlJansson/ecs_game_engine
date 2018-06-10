#pragma once
#include "engine_core.h"
#include "state.h"

namespace lib_graphics {
class GroundedState : public lib_core::State {
 public:
  GroundedState(lib_core::Entity cam_ent, lib_core::EngineCore* engine);
  ~GroundedState() override = default;

  void OnEnter() override;
  void OnExit() override;
  bool Update(float dt) override;

 private:
  lib_core::Entity cam_entity_;
  lib_core::EngineCore* engine_;

  bool in_air = false;
  bool float_jumped = true;
  float jump_cooldown_ = 0.f;
  float min_life_ = 0.f;

  float orig_height_;
  float touch_down_ = 2.f;
};
}  // namespace app_fract_editor
