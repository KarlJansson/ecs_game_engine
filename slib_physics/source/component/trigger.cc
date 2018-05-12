#include "trigger.h"

namespace lib_physics {
Trigger::Trigger(TriggerType t, lib_core::Vector3 pos, lib_core::Vector3 rot,
                 lib_core::Vector3 size) {
  type = t;
  this->pos = std::move(pos);
  this->rot = std::move(rot);
  this->size = std::move(size);
}
}  // namespace lib_physics