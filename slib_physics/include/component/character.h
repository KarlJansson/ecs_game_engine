#pragma once
#include "vector_def.h"

namespace lib_physics {
class Character {
 public:
  Character(lib_core::Vector3 pos = {.0f}, float height = 1.f,
            float radius = .2f, float density = 1.f);
  ~Character() = default;

  void Move(lib_core::Vector3 dp);
  void Teleport(lib_core::Vector3 pos);
  void Resize(float size);

  lib_core::Vector3 disp;
  lib_core::Vector3 pos;
  lib_core::Vector3 port_loc;

  float height;
  float radius;
  float density;
  float vert_velocity = 0;

  bool grounded = false;
  bool teleport = false;
  bool resize = false;
};
}  // namespace lib_physics
