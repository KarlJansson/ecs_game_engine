#include "physics_factory.h"
#include "physx_system.h"

namespace lib_physics {
std::unique_ptr<PhysicsSystem> PhysicsFactory::CreatePhysicsSystem() {
  return std::make_unique<PhysxSystem>();
}
}  // namespace lib_physics
