#pragma once
#include "core_utilities.h"
#include "key_definitions.h"

namespace lib_input {
class ContiguousInput {
 public:
  ct::dyn_array<ct::dyn_array<Key>> key_combos;
  std::function<void(float)> function_binding;
  bool active = true;
};
}  // namespace lib_input