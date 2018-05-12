#include "joint.h"

namespace lib_physics {
Joint::Joint(JointType t, lib_core::Entity a1, lib_core::Entity a2,
             lib_core::Vector3 pos1, lib_core::Vector3 rot1,
             lib_core::Vector3 pos2, lib_core::Vector3 rot2,
             lib_core::Vector2 lim, lib_core::Vector2 break_ft) {
  type = t;
  actor_1 = a1;
  actor_2 = a2;

  pos_1 = std::move(pos1);
  pos_2 = std::move(pos2);

  rot_1 = std::move(rot1);
  rot_2 = std::move(rot2);

  limits = std::move(lim);

  break_force_torque = std::move(break_ft);
}

void Joint::SetLimits(float lim1, float lim2) {
  limits = {lim1, lim2};
  set_limits = true;
}
}  // namespace lib_physics