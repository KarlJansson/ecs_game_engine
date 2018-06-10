#pragma once
#include "system.h"

namespace lib_physics {
class Actor;
}

namespace lib_graphics {
class TransformSystem : public lib_core::System {
 public:
  TransformSystem() = default;
  ~TransformSystem() override = default;

  void LogicUpdate(float dt) override;
  void UpdateTransform(class Transform& trans, class Transform& old,
                       const lib_physics::Actor* actor,
                       const lib_physics::Actor* old_actor);
};
}  // namespace lib_graphics
