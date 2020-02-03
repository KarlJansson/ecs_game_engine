#include "fps_camera_system.h"
#include "actor.h"
#include "camera.h"
#include "character.h"
#include "character_handler.h"
#include "contiguous_input.h"
#include "engine_settings.h"
#include "entity_manager.h"
#include "grounded_state.h"
#include "gui_rect.h"
#include "input_system.h"
#include "joint.h"
#include "light.h"
#include "material_system.h"
#include "physics_system.h"
#include "renderer.h"
#include "shared_state.h"
#include "transform.h"
#include "transform_system.h"
#include "window.h"

namespace lib_graphics {
FpsCameraSystem::FpsCameraSystem(lib_core::EngineCore *engine)
    : engine_(engine) {
  camera_state_graph = std::make_unique<lib_core::StateMachine>(
      lib_core::StateMachine::kMultiState);
  start_location.ZeroMem();
  start_rotation.ZeroMem();
}

void FpsCameraSystem::InitSystem() {
  reticule_entity_1 = CreateScopedEntity();
  reticule_entity_2 = CreateScopedEntity();
  g_ent_mgr.AddComponent<lib_gui::GuiRect>(
      reticule_entity_1,
      lib_gui::GuiRect({.5f, .5f}, {.01f, .01f}, {0.f, 0.f, 0.f, .2f}, 0));
  g_ent_mgr.AddComponent<lib_gui::GuiRect>(
      reticule_entity_2,
      lib_gui::GuiRect({.5f, .5f}, {.009f, .009f}, {1.f, 1.f, 1.f, .2f}, 1));

  cam_entity_ = CreateScopedEntity();
  auto cam_comp = lib_graphics::Camera(start_location, start_rotation);
  cam_comp.SetExposure(cam_exposure_);
  g_ent_mgr.AddComponent<lib_graphics::Camera>(cam_entity_, cam_comp);

  g_ent_mgr.AddComponent<lib_physics::Character>(
      cam_entity_, lib_physics::Character(start_location, camera_height_, .5f));

  right_hand_ = CreateScopedEntity();
  auto rhand_actor = lib_physics::Actor(lib_core::EngineCore::stock_box_mesh,
                                        lib_physics::Actor::kKinematic);
  rhand_actor.scale = lib_core::Vector3(.01f);
  rhand_actor.SetPose({0.0f, -1000.0f, 0.0f}, {0.f});
  g_ent_mgr.AddComponent(right_hand_, rhand_actor);

  lib_input::ContiguousInput mli;
  mli.function_binding = [&](float dt) {
    auto input_system = engine_->GetInput();
    auto mouse_delta = input_system->MouseDelta();
    if (!mouse_delta.Zero()) {
      auto camera_w = g_ent_mgr.GetNewCbeW<lib_graphics::Camera>(cam_entity_);
      if (camera_w) {
        camera_w->Yaw(mouse_delta[0] * 0.001f);
        camera_w->Pitch(mouse_delta[1] * 0.001f);
      }
    }

    auto look_x = input_system->StickPos(0, lib_input::PadStick::kStickRx);
    auto look_y = input_system->StickPos(0, lib_input::PadStick::kStickRy);
    if (std::abs(look_x) + std::abs(look_y) > 0e-4) {
      auto camera_w = g_ent_mgr.GetNewCbeW<lib_graphics::Camera>(cam_entity_);
      if (camera_w) {
        auto yaw_multi = std::abs(look_x) * 3.f;
        auto pitch_multi = std::abs(look_y) * 3.f;

        camera_w->Yaw(look_x * dt * yaw_multi);
        camera_w->Pitch(-look_y * dt * pitch_multi);
      }
    }

    auto transform_system = engine_->TransformSystem();
    auto camera = g_ent_mgr.GetNewCbeR<lib_graphics::Camera>(cam_entity_);
    auto old_camera = g_ent_mgr.GetOldCbeR<lib_graphics::Camera>(cam_entity_);
    if (actor_pickup != -1 && camera && transform_system && !rope_in_ &&
        !pickup_initiated_) {
      if (pickup_type_ == kKinematic) {
        auto rhand_transform =
            g_ent_mgr.GetNewCbeW<lib_graphics::Transform>(right_hand_capture_);
        auto rhand_light =
            g_ent_mgr.GetOldCbeW<lib_graphics::Light>(right_hand_capture_);
        auto cam_character =
            g_ent_mgr.GetNewCbeW<lib_physics::Character>(cam_entity_);

        if (rhand_transform) {
          auto forward = camera->Forward(*old_camera);
          auto left = camera->Left(*old_camera);
          auto down = camera->Up(*old_camera) * -1.f;

          auto new_rotation = camera->Rotation(*old_camera);

          new_rotation[1] -= PI * .7f;
          new_rotation[0] = PI;

          last_pickup_position = new_pickup_position;
          if (cam_character)
            new_pickup_position =
                cam_character->pos + forward * 2.f + left * 1.4f + down * .65f;
          else
            new_pickup_position = camera->Position(*old_camera) +
                                  forward * 2.f + left * 1.4f + down * .65f;
          deltapos_delta_ = dt;
          rhand_transform->MoveTo(new_pickup_position);
          rhand_transform->RotateTo(new_rotation);

          if (rhand_light) rhand_light->data_pos = new_pickup_position;

          auto old_trans = g_ent_mgr.GetOldCbeW<lib_graphics::Transform>(
              right_hand_capture_);
          transform_system->UpdateTransform(*old_trans, *rhand_transform,
                                            nullptr, nullptr);
          //*old_trans = *rhand_transform;
        }
      }
    }
  };

  g_ent_mgr.AddComponent<lib_input::ContiguousInput>(cam_entity_, mli);
  camera_state_graph->PushState(
      std::make_unique<GroundedState>(cam_entity_, engine_));
}

void FpsCameraSystem::FinalizeSystem() {}

void FpsCameraSystem::LogicUpdate(float dt) {
  camera_state_graph->Update(dt);

  auto wind_dim = engine_->GetWindow()->GetWindowDim();
  if (wind_dim != last_window_dim_) {
    auto cam_a_ratio = float(wind_dim.first) / float(wind_dim.second);
    auto cam_w = g_ent_mgr.GetNewCbeW<lib_graphics::Camera>(cam_entity_);
    cam_w->SetAspectRatio(cam_a_ratio);
    last_window_dim_ = wind_dim;
  }

  auto input_system = engine_->GetInput();
  auto physic_system = engine_->GetPhysics();
  auto actor = g_ent_mgr.GetNewCbeR<lib_physics::Character>(cam_entity_);
  auto camera = g_ent_mgr.GetNewCbeR<lib_graphics::Camera>(cam_entity_);

  auto exposure_commands = g_sys_mgr.GetCommands<SetCameraExposure>();
  if (exposure_commands && !exposure_commands->empty()) {
    last_cam_exposure_ = cam_exposure_;
    for (auto &c : *exposure_commands) cam_exposure_ = c.exposure;
    exposure_commands->clear();
  }

  auto start_loc_commands = g_sys_mgr.GetCommands<SetStartLocation>();
  if (start_loc_commands && actor && !start_loc_commands->empty()) {
    for (auto &c : *start_loc_commands) start_location = c.pos;
    start_loc_commands->clear();

    auto actor_w = g_ent_mgr.GetNewCbeW<lib_physics::Character>(cam_entity_);
    actor_w->Teleport(start_location);
  }

  auto start_rot_commands = g_sys_mgr.GetCommands<SetStartRotation>();
  if (start_rot_commands && camera && !start_rot_commands->empty()) {
    for (auto &c : *start_rot_commands) start_rotation = c.rot;
    start_rot_commands->clear();

    auto camera_w = g_ent_mgr.GetNewCbeW<lib_graphics::Camera>(cam_entity_);
    camera_w->SetRotation(start_rotation);
  }

  auto set_height_commands = g_sys_mgr.GetCommands<SetCameraHeight>();
  if (set_height_commands && actor && !set_height_commands->empty()) {
    auto actor_w = g_ent_mgr.GetNewCbeW<lib_physics::Character>(cam_entity_);
    for (auto &c : *set_height_commands) camera_height_ = c.height;
    actor_w->Resize(camera_height_);
    set_height_commands->clear();
  }

  if (camera) {
    if (input_system->KeyPressed(lib_input::Key::kP))
      cam_exposure_ += dt * 5.0f;
    if (input_system->KeyPressed(lib_input::Key::kO))
      cam_exposure_ -= dt * 5.0f;

    if (input_system->KeyPressed(lib_input::Key::kK) ||
        input_system->KeyPressed(lib_input::Key::kL)) {
      auto camera_w = g_ent_mgr.GetNewCbeW<lib_graphics::Camera>(cam_entity_);
      if (input_system->KeyPressed(lib_input::Key::kK))
        camera_w->SetFov(camera_w->fov_ + dt * 50.0f);
      if (input_system->KeyPressed(lib_input::Key::kL))
        camera_w->SetFov(camera_w->fov_ - dt * 50.0f);
    }

    if (cam_exposure_ != last_cam_exposure_) {
      auto camera_w = g_ent_mgr.GetNewCbeW<lib_graphics::Camera>(cam_entity_);
      camera_w->SetExposure(cam_exposure_ < 0.0f ? 0.0f : cam_exposure_);
      last_cam_exposure_ = cam_exposure_;
    }

    auto forward = camera->forward_;
    auto left = camera->left_;

    physic_system->GetRayCastResult(ray_cast_id_, pickup_feeler_);

    lib_physics::PhysicsSystem::RayCastDesc ray_desc;
    ray_desc.origin = camera->position_ + forward * .51f;
    ray_desc.dir = forward;
    ray_desc.max_dist = 500.f;
    ray_desc.dyn_hits = true;
    ray_desc.stat_hits = true;

    ray_cast_id_ = physic_system->RayCast(ray_desc);

    if (rope_in_) {
      rope_length_ -= dt * rope_in_speed_;
      if (rope_length_ < 0.f) {
        rope_length_ = 0.05f;
        rope_in_ = false;

        auto joint_comp = lib_physics::Joint(lib_physics::Joint::kFixed,
                                             right_hand_, right_hand_capture_);
        g_ent_mgr.AddComponent(right_hand_capture_, joint_comp);
      }

      if (rope_in_) {
        auto joint =
            g_ent_mgr.GetNewCbeW<lib_physics::Joint>(right_hand_capture_);
        if (joint) joint->SetLimits(.0f, rope_length_);
      }
    }

    if (joint_removed_) {
      auto rhand_actor = g_ent_mgr.GetNewCbeW<lib_physics::Actor>(right_hand_);
      joint_removed_ = false;
      rhand_actor->SetPose(
          {camera->position_[0], -1000.0f, camera->position_[2]}, {0.f});
      if (apply_force_) {
        hand_moved_ = true;
        apply_force_ = false;
      }
    }

    if (hand_moved_) {
      hand_moved_ = false;
      auto pickup_actor =
          g_ent_mgr.GetNewCbeW<lib_physics::Actor>(right_hand_capture_);

      auto new_velocity = new_pickup_position - last_pickup_position;
      if (pickup_actor) {
        if (!new_velocity.Zero()) {
          auto force_length = new_velocity.Length();
          if (deltapos_delta_ < 1.f)
            force_length /= 1.f - deltapos_delta_;
          else if (deltapos_delta_ > 1.f)
            force_length /= deltapos_delta_;
          new_velocity.Normalize();
          pickup_actor->AddForce(new_velocity * 400.f * force_length);
        }

        if (pickup_feeler_.second > 0.f) {
          auto aim_pos = camera->position_ + forward * pickup_feeler_.second;
          // Overshoot based on distance
          aim_pos +=
              lib_core::Vector3(0.f, 1.f, 0.f) * (pickup_feeler_.second * .1f);
          auto push_dir = aim_pos - pickup_actor->pos;
          push_dir.Normalize();
          pickup_actor->AddForce(push_dir * push_force_);
        } else
          pickup_actor->AddForce(forward * push_force_);
      }
    }

    if (pickup_initiated_) {
      g_ent_mgr.AddComponent(
          right_hand_capture_,
          lib_physics::Joint(lib_physics::Joint::kDistance, right_hand_,
                             right_hand_capture_, {0.f}, {0.f}, {0.f}, {0.f},
                             {.0f, rope_length_}));
      pickup_initiated_ = false;
      rope_in_ = true;
    }

    auto trigger = input_system->StickPos(0, lib_input::PadStick::kTriggerR);
    bool action_a =
        input_system->MousePressed(lib_input::Key::kMouseLeft) ||
        input_system->ButtonPressed(0, lib_input::PadButton::kButtonX);
    bool action_b = input_system->MousePressed(lib_input::Key::kMouseRight) ||
                    trigger > .1f;

    auto gui_rect = g_ent_mgr.GetNewCbeR<lib_gui::GuiRect>(reticule_entity_1);
    if (pickup_feeler_.first != -1) {
      auto pullable = g_ent_mgr.GetNewCbeR<Pullable>(
          lib_core::Entity(pickup_feeler_.first));
      auto pickupable = g_ent_mgr.GetNewCbeR<Pickupable>(
          lib_core::Entity(pickup_feeler_.first));

      if (pullable || pickupable) {
        if (gui_rect && actor_pickup == -1 &&
            gui_rect->rgba_ != lib_core::Vector4(0.f, 1.f, 0.f, .2f)) {
          auto gui_rect_w =
              g_ent_mgr.GetNewCbeW<lib_gui::GuiRect>(reticule_entity_1);
          gui_rect_w->rgba_ = {0.f, 1.f, 0.f, .2f};
        }

        if (!joint_removed_ && actor_pickup == -1 && action_a &&
            !action_block_) {
          if (pickupable) pickup_type_ = kKinematic;
          if (pullable) pickup_type_ = kForce;

          action_block_ = true;
          actor_pickup = pickup_feeler_.first;
          if (pickup_type_ == kKinematic) {
            right_hand_capture_ = lib_core::Entity(actor_pickup);
            rope_length_ = pickup_feeler_.second;
            rope_in_speed_ = rope_length_ * 5.0f;
            pickup_initiated_ = true;
          }
        }
      } else {
        if (gui_rect &&
            gui_rect->rgba_ != lib_core::Vector4(0.f, 0.f, 0.f, .2f)) {
          auto gui_rect_w =
              g_ent_mgr.GetNewCbeW<lib_gui::GuiRect>(reticule_entity_1);
          gui_rect_w->rgba_ = {0.f, 0.f, 0.f, .2f};
        }
      }
    } else {
      if (gui_rect &&
          gui_rect->rgba_ != lib_core::Vector4(0.f, 0.f, 0.f, .2f)) {
        auto gui_rect_w =
            g_ent_mgr.GetNewCbeW<lib_gui::GuiRect>(reticule_entity_1);
        gui_rect_w->rgba_ = {0.f, 0.f, 0.f, .2f};
      }
    }

    if (actor_pickup != -1 && camera) {
      if (pickup_type_ == kKinematic) {
        if (action_a && !rope_in_ && !pickup_initiated_) {
          g_ent_mgr.RemoveComponent<lib_physics::Joint>(right_hand_capture_);

          push_force_ = .1f;
          joint_removed_ = true;
          apply_force_ = true;
          rope_in_ = false;
          action_block_ = true;

          actor_pickup = -1;
        } else if (action_b && !rope_in_ && !pickup_initiated_) {
          g_ent_mgr.RemoveComponent<lib_physics::Joint>(right_hand_capture_);

          push_force_ = 500.f;
          joint_removed_ = true;
          apply_force_ = true;
          rope_in_ = false;
          action_block_ = true;

          actor_pickup = -1;
        } else {
          auto rhand_actor =
              g_ent_mgr.GetNewCbeW<lib_physics::Actor>(right_hand_);
          auto rhand_capture =
              g_ent_mgr.GetNewCbeW<lib_physics::Joint>(right_hand_capture_);
          if (rhand_capture && rhand_capture->broken) {
            g_ent_mgr.RemoveComponent<lib_physics::Joint>(right_hand_capture_);
            joint_removed_ = true;
            rope_in_ = false;
            push_force_ = .2f;
            apply_force_ = true;

            actor_pickup = -1;
          } else if (rhand_actor) {
            auto forward = camera->forward_;
            auto left = camera->left_;
            auto down = camera->up_ * -1.f;

            auto new_rotation = camera->rotation_;

            new_rotation[1] -= PI * .7f;
            new_rotation[0] = PI;

            if (actor) {
              last_pickup_position = new_pickup_position;
              new_pickup_position =
                  camera->position_ + forward * 2.f + left * 1.4f + down * .65f;
              deltapos_delta_ = dt;
              rhand_actor->SetPose(new_pickup_position,
                                   new_rotation * (180.f / PI));
            } else {
              last_pickup_position = new_pickup_position;
              new_pickup_position =
                  camera->position_ + forward * 2.f + left * 1.4f + down * .65f;
              deltapos_delta_ = dt;
              rhand_actor->SetPose(new_pickup_position,
                                   new_rotation * (180.f / PI));
            }
          }
        }
      } else {
        if (!action_block_ && (action_a || action_b)) {
          actor_pickup = -1;
          action_block_ = true;
        } else {
          float multiple = 2;
          auto rhand_capture_actor = g_ent_mgr.GetNewCbeW<lib_physics::Actor>(
              lib_core::Entity(actor_pickup));
          if (rhand_capture_actor) {
            auto down = camera->up_ * -1.f;
            rhand_capture_actor->AddForce(
                ((camera->position_ + forward * 1.5f + left + down * .4f) -
                 rhand_capture_actor->pos) *
                multiple);
          }
        }
      }
    }

    if (!action_a & !action_b) action_block_ = false;
  }
}

lib_core::Entity FpsCameraSystem::GetCamEntity() { return cam_entity_; }
}  // namespace lib_graphics
