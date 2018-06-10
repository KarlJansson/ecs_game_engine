#include "character.h"
#include <utility>

namespace lib_physics {
Character::Character(lib_core::Vector3 pos, float height, float radius,
                     float density)
    : pos(pos), height(height), radius(radius), density(density) {
  disp.ZeroMem();
}

void Character::Move(lib_core::Vector3 dp) { disp += dp; }

void Character::Teleport(lib_core::Vector3 pos) {
  port_loc = pos;
  teleport = true;
}

void Character::Resize(float size) {
  height = size;
  resize = true;
}
}  // namespace lib_physics
