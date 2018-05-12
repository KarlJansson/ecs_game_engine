#pragma once
#include "key_definitions.h"

namespace lib_input {
class ToggleInput {
 public:
  ct::dyn_array<ct::dyn_array<Key>> key_combos;
  std::function<void(float)> function_binding;
  bool toggled = false;
  bool active = true;
};
}  // namespace lib_input