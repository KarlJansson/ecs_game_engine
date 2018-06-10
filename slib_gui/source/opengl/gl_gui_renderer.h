#pragma once
#include "gui_renderer.h"

namespace lib_gui {
class GlGuiRenderer : public GuiRenderer {
 public:
  GlGuiRenderer(lib_core::EngineCore* engine);
  ~GlGuiRenderer() override = default;

  void Draw(lib_core::Vector2 screen_dim) override;

 private:
  struct SortedCompStruct {
    ct::dyn_array<size_t> texts;
    ct::dyn_array<size_t> rects;
  };

  ct::tree_map<uint8_t, SortedCompStruct> sorted_comps_;
  int shader_locs_[1];
};
}  // namespace lib_gui
