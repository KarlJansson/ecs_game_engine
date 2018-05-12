#pragma once
#include "gui_rect.h"

namespace lib_gui {
class GuiMouseArea {
 public:
  GuiMouseArea() = default;
  ~GuiMouseArea() = default;

  GuiRect rect_;
};
}  // namespace lib_gui
