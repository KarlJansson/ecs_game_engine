#pragma once
#include "entity.h"
#include "vector_def.h"

namespace lib_physics {
class Trigger {
 public:
  enum TriggerType { kBox, kSphere };

  Trigger() {}
  Trigger(TriggerType t, lib_core::Vector3 pos = {.0f},
          lib_core::Vector3 rot = {.0f}, lib_core::Vector3 size = {1.f});
  ~Trigger() = default;

  TriggerType type;
  lib_core::Vector3 pos;
  lib_core::Vector3 rot;
  lib_core::Vector3 size;
  std::function<void(lib_core::Entity)> trigger_callback;
};
}  // namespace lib_physics
