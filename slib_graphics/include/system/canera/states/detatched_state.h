#pragma once
#include "actor.h"
#include "character.h"
#include "engine_core.h"
#include "state.h"

namespace lib_graphics {
class DetatchedState : public lib_core::State {
 public:
  DetatchedState(lib_core::Entity cam_ent, lib_core::EngineCore* engine);
  ~DetatchedState() override = default;

  void OnEnter() override;
  void OnExit() override;
  bool Update(float dt) override;

 private:
  lib_core::EngineCore* engine_;

  float min_life_ = 0.f;
  lib_physics::Character cam_actor;

  lib_core::Entity cam_entity_;
};
}  // namespace app_fract_editor
