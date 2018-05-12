#pragma once
#include <memory>
#include "PxPhysicsAPI.h"
#include "character_handler.h"
#include "core_utilities.h"
#include "entity.h"

namespace lib_physics {
class PhysxCharacterHandler : public CharacterHandler {
 public:
  PhysxCharacterHandler(physx::PxPhysics* phys, physx::PxScene* scene);
  ~PhysxCharacterHandler();

  void UpdateCharacters(float dt);

 private:
  void CreateCharacterController(lib_core::Entity ent);
  void RemoveCharacterController(lib_core::Entity ent);

  class PhysxReportCallback : public physx::PxUserControllerHitReport {
   public:
    void onShapeHit(const physx::PxControllerShapeHit& hit) override;
    void onControllerHit(const physx::PxControllersHit& hit) override;
    void onObstacleHit(const physx::PxControllerObstacleHit& hit) override;
  };

  ct::dyn_array<lib_core::Entity> add_character_, remove_character_;

  size_t add_callback_, remove_callback_;
  physx::PxPhysics* physics_ = nullptr;

  physx::PxControllerManager* controller_manager_ = nullptr;

  ct::hash_map<lib_core::Entity, physx::PxController*> controllers_;
  std::unique_ptr<PhysxReportCallback> report_callback;
};
}  // namespace lib_physics
