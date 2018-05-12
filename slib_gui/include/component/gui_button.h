#pragma once
#include "vector_def.h"

namespace lib_gui {
class GuiButton {
 public:
  GuiButton() = default;
  ~GuiButton() = default;

  lib_core::Vector2 location;
  lib_core::Vector2 half_size;
};
}  // namespace lib_gui
