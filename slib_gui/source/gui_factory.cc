#include "gui_factory.h"
#include "gl_gui_renderer.h"
#include "gl_rect_system.h"
#include "gl_text_system.h"

namespace lib_gui {
std::unique_ptr<GuiRenderer> GuiFactory::CreateGuiRenderer(
    lib_core::EngineCore* engine) {
  return std::make_unique<GlGuiRenderer>(engine);
}

std::unique_ptr<RectSystem> GuiFactory::CreateRectSystem() {
  return std::make_unique<GlRectSystem>();
}

std::unique_ptr<TextSystem> GuiFactory::CreateTextSystem() {
  return std::make_unique<GlTextSystem>();
}
}  // namespace lib_gui
