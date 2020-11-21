#pragma once
#include "system.h"

namespace lib_physics {
class Actor;
class Character;
}  // namespace lib_physics

namespace lib_graphics {
class TransformSystem : public lib_core::System {
 public:
  TransformSystem() = default;
  ~TransformSystem() override = default;

  void LogicUpdate(float dt) override;
  void UpdateTransform(class Transform& trans, class Transform& old,
                       const lib_physics::Actor* actor,
                       const lib_physics::Character* character);
};
}  // namespace lib_graphics
