#pragma once
#include "gui_rect.h"

namespace lib_gui {
class GuiProgressBar {
 public:
  GuiProgressBar() = default;
  ~GuiProgressBar() = default;

  GuiRect bg_rect_, progress_rect_;
};
}  // namespace lib_gui
