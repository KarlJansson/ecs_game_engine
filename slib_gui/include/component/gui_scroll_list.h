#pragma once
#include "gui_mouse_area.h"
#include "gui_rect.h"

namespace lib_gui {
class GuiScrollList {
 public:
  GuiScrollList() = default;
  ~GuiScrollList() = default;

  GuiMouseArea scroll_area_;
  GuiRect scroll_bg_rect_, scroll_rect_, scroll_pane_rect_;
};
}  // namespace lib_gui
