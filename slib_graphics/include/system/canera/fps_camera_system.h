#pragma once
#include "character.h"
#include "engine_core.h"
#include "entity.h"
#include "state_machine.h"
#include "system.h"
#include "system_manager.h"

namespace lib_graphics {
class FpsCameraSystem : public lib_core::System {
 public:
  FpsCameraSystem(lib_core::EngineCore *engine);
  ~FpsCameraSystem() = default;

  // System component tags
  class Pullable {
   public:
    Pullable() = default;
  };

  class Pickupable {
   public:
    Pickupable() = default;
  };

  void InitSystem() override;
  void FinalizeSystem() override;

  void LogicUpdate(float dt) override;

  enum PickupType { kKinematic, kForce };

  lib_core::Entity GetCamEntity();

  class SetCameraHeight : public lib_core::Command {
   public:
    SetCameraHeight() = default;
    SetCameraHeight(float height) : height(height) {}
    float height;
  };

  class SetStartLocation : public lib_core::Command {
   public:
    SetStartLocation() = default;
    SetStartLocation(lib_core::Vector3 pos) : pos(pos) {}
    lib_core::Vector3 pos;
  };

  class SetStartRotation : public lib_core::Command {
   public:
    SetStartRotation() = default;
    SetStartRotation(lib_core::Vector3 rot) : rot(rot) {}
    lib_core::Vector3 rot;
  };

  class SetCameraExposure : public lib_core::Command {
   public:
    SetCameraExposure() = default;
    SetCameraExposure(float exposure) : exposure(exposure) {}
    float exposure;
  };

 private:
  lib_core::EngineCore *engine_;
  lib_core::Entity cam_entity_;

  float cam_exposure_ = 0.0f;
  float last_cam_exposure_ = 0.0f;

  float camera_height_ = 1.0f;

  lib_core::Vector3 start_location;
  lib_core::Vector3 start_rotation;

  lib_core::Vector3 last_pickup_position;
  lib_core::Vector3 new_pickup_position;
  float deltapos_delta_ = 0.f;

  lib_physics::Character cam_actor;

  lib_core::Entity right_hand_;
  lib_core::Entity right_hand_capture_;

  PickupType pickup_type_;
  int actor_pickup = -1;

  bool rope_in_ = false;
  float rope_in_speed_;
  float rope_length_;

  bool joint_removed_ = false;
  bool hand_moved_ = false;
  bool apply_force_ = false;
  bool action_block_ = false;
  bool pickup_initiated_ = false;

  std::pair<int, int> last_window_dim_;

  uint32_t ray_cast_id_;
  float push_force_;

  std::pair<int, float> pickup_feeler_;

  lib_core::Entity phys_id_tmp_, reticule_entity_1, reticule_entity_2;

  std::unique_ptr<lib_core::StateMachine> camera_state_graph;
};
}  // namespace lib_graphics
