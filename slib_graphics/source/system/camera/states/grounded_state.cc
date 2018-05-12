#include "grounded_state.h"
#include "camera.h"
#include "character.h"
#include "contiguous_input.h"
#include "detatched_state.h"
#include "input_system.h"
#include "state_machine.h"
#include "toggle_input.h"

namespace lib_graphics {
GroundedState::GroundedState(lib_core::Entity cam_ent,
                             lib_core::EngineCore* engine)
    : cam_entity_(cam_ent), engine_(engine) {}

void GroundedState::OnEnter() {
  auto movement_input = CreateScopedEntity();

  lib_input::ContiguousInput input_comp;
  input_comp.function_binding = [&](float dt) {
    if (!engine_) return;

    auto input_system = engine_->GetInput();
    auto actor = g_ent_mgr.GetNewCbeR<lib_physics::Character>(cam_entity_);
    auto camera = g_ent_mgr.GetNewCbeR<lib_graphics::Camera>(cam_entity_);

    if (actor && camera) {
      auto add = 9.f;

      bool run = input_system->KeyPressed(lib_input::kLeftShift) ||
                 input_system->ButtonPressed(0, 4);

      if (run) add += 9.f;

      auto xz_forward = camera->forward_;
      xz_forward[1] = 0.0f;
      xz_forward.Normalize();

      auto xz_left = camera->left_;
      xz_left[1] = 0.0f;
      xz_left.Normalize();

      lib_core::Vector3 move_delta;
      move_delta.ZeroMem();
      if (input_system->KeyPressed(lib_input::kW))
        move_delta += xz_forward * add;
      if (input_system->KeyPressed(lib_input::kS))
        move_delta += xz_forward * -add;
      if (input_system->KeyPressed(lib_input::kD)) move_delta += xz_left * add;
      if (input_system->KeyPressed(lib_input::kA)) move_delta += xz_left * -add;

      auto move_stick = input_system->StickPos(0, 0);
      if (!move_stick.Zero()) {
        auto forward_multi = std::abs(move_stick[1]) * add;
        auto left_multi = std::abs(move_stick[0]) * add;

        move_delta += xz_forward * -move_stick[1] * forward_multi;
        move_delta += xz_left * move_stick[0] * left_multi;
      }

      if (move_delta.Length() > add) {
        move_delta.Normalize();
        move_delta *= add;
      }

      move_delta *= dt;

      bool jump = input_system->KeyPressed(lib_input::kSpace) ||
                  input_system->ButtonPressed(0, 0);

      if (actor->grounded && in_air && jump_cooldown_ >= 0.3f) {
        orig_height_ = actor->height;
        touch_down_ = 0.f;
        in_air = false;
      }

      if (touch_down_ < 1.f) {
        auto actor_w =
            g_ent_mgr.GetNewCbeW<lib_physics::Character>(cam_entity_);
        touch_down_ += dt * 5.f;
        touch_down_ = std::fmin(touch_down_, 1.f);
        actor_w->Resize(co::lerp(orig_height_, orig_height_ * .97f,
                                 co::sine_fn(touch_down_)));
      }

      if (jump_cooldown_ < .3f) jump_cooldown_ += dt;
      if (jump && (actor->grounded || !float_jumped) &&
          jump_cooldown_ >= 0.3f) {
        if (touch_down_ < 1.f) {
          auto actor_w =
              g_ent_mgr.GetNewCbeW<lib_physics::Character>(cam_entity_);
          actor_w->Resize(orig_height_);
          touch_down_ = 1.1f;
        }

        in_air = true;
        move_delta[1] += 15.0f;
        if (!actor->grounded) float_jumped = true;
        jump_cooldown_ = 0.0f;
      }

      // if (actor->grounded) float_jumped = false;

      if (!move_delta.Zero()) {
        auto actor_w =
            g_ent_mgr.GetNewCbeW<lib_physics::Character>(cam_entity_);
        actor_w->Move(move_delta);
      }
    }
  };
  g_ent_mgr.AddComponent(movement_input, input_comp);
}

void GroundedState::OnExit() {}

bool GroundedState::Update(float dt) {
  auto input_system = engine_->GetInput();
  auto actor = g_ent_mgr.GetNewCbeR<lib_physics::Character>(cam_entity_);

  if (actor) {
    if (min_life_ >= .5f && input_system->KeyPressed(lib_input::kLeftControl) &&
        input_system->KeyPressed(lib_input::kE)) {
      owner_->ReplaceTopState(
          std::make_unique<DetatchedState>(cam_entity_, engine_));
      return true;
    }
    if (min_life_ < .5f) min_life_ += dt;
  }
  return true;
}
}  // namespace app_fract_editor
