#pragma once
#include <memory>
#include "PxPhysicsAPI.h"
#include "core_utilities.h"
#include "entity.h"

namespace lib_physics {
class PhysxTriggerHandler {
 public:
  PhysxTriggerHandler(physx::PxPhysics* physics, physx::PxScene* scene);
  ~PhysxTriggerHandler();

  void Update();

 private:
  physx::PxPhysics* physics_;
  physx::PxScene* scene_;

  void AddTrigger(lib_core::Entity e);
  void RemoveTrigger(lib_core::Entity e);

  class PhysxTriggerCallback : public physx::PxSimulationEventCallback {
   public:
    void onAdvance(const physx::PxRigidBody* const* bodyBuffer,
                   const physx::PxTransform* poseBuffer,
                   const physx::PxU32 count) override {}
    void onContact(const physx::PxContactPairHeader& pairHeader,
                   const physx::PxContactPair* pairs,
                   physx::PxU32 nbPairs) override {}
    void onConstraintBreak(physx::PxConstraintInfo* constraints,
                           physx::PxU32 count) override {}
    void onWake(physx::PxActor** actors, physx::PxU32 count) override {}
    void onSleep(physx::PxActor** actors, physx::PxU32 count) override {}
    void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;

    ct::hash_map<lib_core::Entity, std::function<void(lib_core::Entity)>>
        trigger_map;
  };

  size_t add_trigger_callback_, remove_trigger_callback_;
  ct::dyn_array<lib_core::Entity> add_triggers_, remove_triggers_;

  ct::hash_map<lib_core::Entity, physx::PxRigidActor*> triggers_;
  std::unique_ptr<PhysxTriggerCallback> trigger_callback;
  physx::PxMaterial* default_material_;
};
}  // namespace lib_physics
