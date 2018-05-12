#pragma once
#include "gui_rect.h"
#include "rect_system.h"
#include "vector_def.h"

namespace lib_gui {
class GlRectSystem : public RectSystem {
 public:
  GlRectSystem();
  ~GlRectSystem();

  void DrawUpdate(lib_graphics::Renderer *renderer,
                  lib_gui::TextSystem *text_renderer) override;

  void DrawRect(GuiRect &rect, lib_core::Vector2 screen_dim);
  void PurgeGpuResources() override;

 private:
  void CreateRectResources();

  bool init_draw_ = false;
  unsigned rect_vao_, rect_vbo_, shader_;
  int color_loc_, proj_loc_;
};
}  // namespace lib_gui
