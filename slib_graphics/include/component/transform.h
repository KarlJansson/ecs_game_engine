#pragma once
#include "core_utilities.h"
#include "matrix4x4.h"
#include "vector_def.h"

namespace lib_graphics {
class Transform {
 public:
  Transform(lib_core::Vector3 pos = {0.f, -1000.f, 0.f},
            lib_core::Vector3 rot = {0.f}, lib_core::Vector3 scale = {1.f},
            lib_core::Vector3 orb = {0.f}, lib_core::Vector3 orb_rot = {0.f});

  lib_core::Vector3 Position() const;

  void MoveForward(float amount);
  void MoveBackward(float amount);
  void MoveUp(float amount);
  void MoveDown(float amount);
  void MoveLeft(float amount);
  void MoveRight(float amount);

  void Move(lib_core::Vector3 dir);

  void MoveTo(lib_core::Vector3 pos);
  void RotateTo(lib_core::Vector3 rot);

  void Yaw(float amount);
  void Pitch(float amount);
  void Roll(float amount);

  void ScaleX(float amount);
  void ScaleY(float amount);
  void ScaleZ(float amount);

  lib_core::Vector3 position_, rotation_, scale_, orbit_offset_,
      orbit_rotation_;
  lib_core::Vector3 left_, up_, forward_;
  lib_core::Matrix4x4 world_;

  static Transform Parse(ct::dyn_array<char> &buffer, size_t &cursor,
                         ct::hash_map<ct::string, ct::string> &val_map) {
    lib_core::Vector3 pos = {0.f}, rot = {0.f}, scale = {1.f}, orb_off = {0.f},
                      orb_rot = {0.f};
    if (cu::ScrollCursor(buffer, cursor, '{')) {
      auto type = cu::ParseType(buffer, cursor);
      while (!type.empty()) {
        if (type.compare("Position") == 0)
          pos = cu::ParseVector<lib_core::Vector3, 3>(
              cu::ParseValue(buffer, cursor, val_map));
        else if (type.compare("Rotation") == 0)
          rot = cu::ParseVector<lib_core::Vector3, 3>(
              cu::ParseValue(buffer, cursor, val_map));
        else if (type.compare("Scale") == 0)
          scale = cu::ParseVector<lib_core::Vector3, 3>(
              cu::ParseValue(buffer, cursor, val_map));
        else if (type.compare("OrbitOffset") == 0)
          orb_off = cu::ParseVector<lib_core::Vector3, 3>(
              cu::ParseValue(buffer, cursor, val_map));
        else if (type.compare("OrbitRotation") == 0)
          orb_rot = cu::ParseVector<lib_core::Vector3, 3>(
              cu::ParseValue(buffer, cursor, val_map));

        type = cu::ParseType(buffer, cursor);
      }
    }
    return Transform(pos, rot, scale, orb_off, orb_rot);
  }
};
}  // namespace lib_graphics
