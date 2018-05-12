#pragma once
#include "engine_core.h"
#include "gui_renderer.h"
#include "rect_system.h"
#include "text_system.h"

namespace lib_gui {
class GuiFactory {
 public:
  GuiFactory() = default;
  ~GuiFactory() = default;

  std::unique_ptr<GuiRenderer> CreateGuiRenderer(lib_core::EngineCore* engine);

  std::unique_ptr<RectSystem> CreateRectSystem();
  std::unique_ptr<TextSystem> CreateTextSystem();
};
}  // namespace lib_gui