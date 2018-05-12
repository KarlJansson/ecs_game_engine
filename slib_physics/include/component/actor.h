#pragma once
#include "matrix4x4.h"
#include "quaternion.h"
#include "vector_def.h"

namespace lib_physics {
class Actor {
 public:
  enum ActorType { kStatic, kDynamic, kKinematic, kPlane };

  Actor() = default;
  explicit Actor(size_t meshid, ActorType t, bool ccd = false);
  ~Actor() = default;

  void Move(lib_core::Vector3 dp);

  void SetPose(lib_core::Vector3 p, lib_core::Vector3 r);
  void MovePose(lib_core::Vector3 p, lib_core::Vector3 r);

  void AddForce(lib_core::Vector3 force);
  void AddTorque(lib_core::Vector3 torque);
  void SetVelocity(lib_core::Vector3 velocity);

  ActorType type;
  size_t mesh_id;

  float density = 1.f;
  float static_friction = 1.f;
  float dynamic_friction = 1.f;
  float restitution = .1f;

  lib_core::Vector3 new_pos;
  lib_core::Vector3 new_rot;
  lib_core::Vector3 delta_pos;
  lib_core::Vector3 force;
  lib_core::Vector3 torque;
  lib_core::Vector3 velocity;

  lib_core::Vector3 pos = {.0f, -1000.f, 0.f};
  lib_core::Vector3 scale = {1.f};
  lib_core::Quaternion rot = {.0f, .0f, .0f, 1.f};

  bool ccd_collision;

  bool convex = true;
  bool set_velocity = false;
  bool set_pose = false;
  bool move_pose = false;
};
}  // namespace lib_physics
