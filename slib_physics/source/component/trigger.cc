#include "trigger.h"

namespace lib_physics {
Trigger::Trigger(TriggerType t, lib_core::Vector3 pos, lib_core::Vector3 rot,
                 lib_core::Vector3 size) {
  type = t;
  this->pos = pos;
  this->rot = rot;
  this->size = size;
}
}  // namespace lib_physics