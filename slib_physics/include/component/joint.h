#pragma once
#include "core_utilities.h"
#include "entity.h"
#include "vector_def.h"

namespace lib_physics {
class Joint {
 public:
  enum JointType { kRevolute, kDistance, kFixed, kSpherical, kPrismatic };

  Joint() = default;
  explicit Joint(JointType t, lib_core::Entity a1, lib_core::Entity a2,
                 lib_core::Vector3 pos1 = {.0f}, lib_core::Vector3 rot1 = {.0f},
                 lib_core::Vector3 pos2 = {.0f}, lib_core::Vector3 rot2 = {.0f},
                 lib_core::Vector2 lim = {.0f},
                 lib_core::Vector2 break_ft = {.0f});
  ~Joint() = default;

  void SetLimits(float lim1, float lim2);

  JointType type = kDistance;
  lib_core::Entity actor_1, actor_2;
  lib_core::Vector2 limits = {0.f};
  lib_core::Vector2 break_force_torque = {0.f};
  lib_core::Vector3 pos_1 = {0.f}, pos_2 = {0.f};
  lib_core::Vector3 rot_1 = {0.f}, rot_2 = {0.f};

  bool broken = false;
  bool set_limits = false;

  static Joint Parse(ct::dyn_array<char> &buffer, size_t &cursor,
                     ct::hash_map<ct::string, ct::string> &val_map) {
    Joint j;
    if (cu::ScrollCursor(buffer, cursor, '{')) {
      auto type = cu::ParseType(buffer, cursor);
      while (!type.empty()) {
        auto val = cu::ParseValue(buffer, cursor, val_map);
        if (type.compare("Limits") == 0) {
          j.limits = cu::ParseVector<lib_core::Vector2, 2>(val);
        } else if (type.compare("Type") == 0) {
          if (val.compare("kRevolute") == 0)
            type = kRevolute;
          else if (val.compare("kDistance") == 0)
            type = kDistance;
          else if (val.compare("kFixed") == 0)
            type = kFixed;
          else if (val.compare("kSpherical") == 0)
            type = kSpherical;
          else if (val.compare("kPrismatic") == 0)
            type = kPrismatic;
        } else if (type.compare("BreakLimits") == 0) {
          j.break_force_torque = cu::ParseVector<lib_core::Vector2, 2>(val);
        } else if (type.compare("Position1") == 0) {
          j.pos_1 = cu::ParseVector<lib_core::Vector3, 3>(val);
        } else if (type.compare("Position2") == 0) {
          j.pos_2 = cu::ParseVector<lib_core::Vector3, 3>(val);
        } else if (type.compare("Rotation1") == 0) {
          j.rot_1 = cu::ParseVector<lib_core::Vector3, 3>(val);
        } else if (type.compare("Rotation2") == 0) {
          j.rot_2 = cu::ParseVector<lib_core::Vector3, 3>(val);
        }

        type = cu::ParseType(buffer, cursor);
      }
    }

    return j;
  }
};
}  // namespace lib_physics
