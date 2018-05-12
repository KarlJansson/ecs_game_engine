#include "detatched_state.h"
#include "camera.h"
#include "character.h"
#include "contiguous_input.h"
#include "fps_camera_system.h"
#include "grounded_state.h"
#include "input_system.h"
#include "state_machine.h"

namespace lib_graphics {
DetatchedState::DetatchedState(lib_core::Entity cam_ent,
                               lib_core::EngineCore* engine)
    : engine_(engine), cam_entity_(cam_ent) {}

void DetatchedState::OnEnter() {
  auto actor = g_ent_mgr.GetNewCbeR<lib_physics::Character>(cam_entity_);
  cam_actor = *actor;
  g_ent_mgr.RemoveComponent<lib_physics::Character>(cam_entity_);

  lib_input::ContiguousInput move_comp;
  move_comp.function_binding = [&](float dt) {
    auto input_system = engine_->GetInput();
    auto camera = g_ent_mgr.GetNewCbeR<lib_graphics::Camera>(cam_entity_);
    auto add = 35.0f;
    if (input_system->KeyPressed(lib_input::kLeftShift)) add += 35.0f;
    auto up = camera->up_;

    lib_core::Vector3 move_delta;
    move_delta.ZeroMem();

    if (input_system->KeyPressed(lib_input::kW))
      move_delta += camera->forward_ * dt * add;
    if (input_system->KeyPressed(lib_input::kS))
      move_delta += camera->forward_ * -dt * add;
    if (input_system->KeyPressed(lib_input::kD))
      move_delta += camera->left_ * dt * add;
    if (input_system->KeyPressed(lib_input::kA))
      move_delta += camera->left_ * -dt * add;
    if (input_system->KeyPressed(lib_input::kSpace))
      move_delta += up * dt * add;
    if (input_system->KeyPressed(lib_input::kC)) move_delta += up * -dt * add;

    if (!move_delta.Zero()) {
      auto camera_w = g_ent_mgr.GetNewCbeW<lib_graphics::Camera>(cam_entity_);
      camera_w->Move(move_delta);
    }
  };
  g_ent_mgr.AddComponent(CreateScopedEntity(), move_comp);
}

void DetatchedState::OnExit() {
  auto camera = g_ent_mgr.GetNewCbeR<lib_graphics::Camera>(cam_entity_);
  cam_actor.Teleport(camera->position_);
  g_ent_mgr.AddComponent<lib_physics::Character>(cam_entity_, cam_actor);
  issue_command(lib_graphics::FpsCameraSystem::SetCameraHeight(cam_actor.height));
}

bool DetatchedState::Update(float dt) {
  auto input_system = engine_->GetInput();
  if (min_life_ >= .5f && input_system->KeyPressed(lib_input::kLeftControl) &&
      input_system->KeyPressed(lib_input::kE)) {
    owner_->ReplaceTopState(
        std::make_unique<GroundedState>(cam_entity_, engine_));
    return true;
  }
  if (min_life_ < .5f) min_life_ += dt;
  return true;
}
}  // namespace app_fract_editor
