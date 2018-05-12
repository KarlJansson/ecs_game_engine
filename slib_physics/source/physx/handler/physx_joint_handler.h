#pragma once
#include "joint_handler.h"
#include "physx_actor_handler.h"

namespace lib_physics {
class PhysxJointHandler : public JointHandler {
 public:
  PhysxJointHandler(physx::PxPhysics* phys, PhysxActorHandler* actor_handler);
  ~PhysxJointHandler();

  void Update();

 private:
  bool AddNewJointData(lib_core::Entity entity);
  bool RemoveJointData(lib_core::Entity entity);

  ct::hash_map<lib_core::Entity, physx::PxJoint*> joints_;
  physx::PxPhysics* physics_;
  PhysxActorHandler* actor_handler_;

  ct::dyn_array<lib_core::Entity> add_joint_;
  ct::dyn_array<lib_core::Entity> remove_joint_;

  size_t joint_added_callback, joint_removed_callback;
};
}  // namespace lib_physics
