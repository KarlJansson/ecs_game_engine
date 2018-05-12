#include "gl_gui_renderer.h"
#include "engine_core.h"
#include "entity_manager.h"
#include "gl_rect_system.h"
#include "gl_text_system.h"
#include "gui_text.h"

namespace lib_gui {
GlGuiRenderer::GlGuiRenderer(lib_core::EngineCore* engine)
    : GuiRenderer(engine) {
  shader_locs_[0] = -1;
}

void GlGuiRenderer::Draw(lib_core::Vector2 screen_dim) {
  auto rect_sys = static_cast<GlRectSystem*>(engine_->GetRect());
  auto text_sys = engine_->GetText();

  for (auto& s : sorted_comps_) {
    s.second.rects.clear();
    s.second.texts.clear();
  }

  auto rect_comps = g_ent_mgr.GetOldCbt<lib_gui::GuiRect>();
  auto text_comps = g_ent_mgr.GetOldCbt<lib_gui::GuiText>();
  if (text_comps)
    for (size_t i = 0; i < text_comps->size(); ++i)
      sorted_comps_[(*text_comps)[i].layer].texts.push_back(i);
  if (rect_comps)
    for (size_t i = 0; i < rect_comps->size(); ++i)
      sorted_comps_[(*rect_comps)[i].layer].rects.push_back(i);

  for (auto& layer : sorted_comps_) {
    for (auto& rect : layer.second.rects)
      rect_sys->DrawRect((*rect_comps)[rect], screen_dim);

    for (auto& text : layer.second.texts)
      text_sys->RenderText((*text_comps)[text], screen_dim);
  }
}
}  // namespace lib_gui