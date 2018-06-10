#include "actor.h"
#include <utility>

namespace lib_physics {
Actor::Actor(size_t meshid, ActorType t, bool ccd) {
  force.ZeroMem();
  torque.ZeroMem();
  velocity.ZeroMem();
  delta_pos.ZeroMem();
  mesh_id = meshid;
  type = t;
  ccd_collision = ccd;
}

void Actor::Move(lib_core::Vector3 dp) { delta_pos += dp; }

void Actor::SetPose(lib_core::Vector3 p, lib_core::Vector3 r) {
  new_pos = p;
  new_rot = r;
  set_pose = true;
}

void Actor::MovePose(lib_core::Vector3 p, lib_core::Vector3 r) {
  new_pos = p;
  new_rot = r;
  move_pose = true;
}

void Actor::AddForce(lib_core::Vector3 force) { this->force += force; }

void Actor::AddTorque(lib_core::Vector3 torque) { this->torque += torque; }

void Actor::SetVelocity(lib_core::Vector3 velocity) {
  this->velocity = velocity;
  set_velocity = true;
}
}  // namespace lib_physics
