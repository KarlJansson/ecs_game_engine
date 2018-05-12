#pragma once
#include "physics_commands.h"

namespace lib_physics {
class ActorHandler {
 public:
  ActorHandler() = default;
  ~ActorHandler() = default;

 protected:
  ct::hash_map<size_t, lib_physics::PhysicsInit> mesh_sources_;

  friend class PhysicsSystem;
};
}  // namespace lib_physics