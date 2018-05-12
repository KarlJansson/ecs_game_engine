#pragma once
#include "physics_system.h"
#include "vector_def.h"

namespace lib_physics {
class PhysicsFactory {
 public:
  PhysicsFactory() = default;
  ~PhysicsFactory() = default;

  std::unique_ptr<PhysicsSystem> CreatePhysicsSystem();
};
}  // namespace lib_physics
