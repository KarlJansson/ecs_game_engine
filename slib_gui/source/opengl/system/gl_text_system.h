#pragma once
#include "gui_text.h"
#include "text_system.h"

namespace lib_gui {
class GlTextSystem : public TextSystem {
 public:
  GlTextSystem() = default;
  ~GlTextSystem() override = default;

  void DrawUpdate(lib_graphics::Renderer *renderer,
                  lib_gui::TextSystem *text_renderer) override;
  void RenderText(GuiText text, lib_core::Vector2 screen_dim) override;

  void PurgeGpuResources() override;

 private:
  void HandleUnloadCommand(UnloadFontCommand &command) override;
  void HandleLoadCommand(LoadFontCommand &command) override;

  void CreateTextResources();

  bool init_draw_ = false;
  unsigned text_vao, text_vbo, shader_;
  int color_loc_, proj_loc_;
};
}  // namespace lib_gui
