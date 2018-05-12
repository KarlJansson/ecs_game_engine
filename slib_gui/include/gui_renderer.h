#pragma once
#include "engine_core.h"
#include "rect_system.h"
#include "text_system.h"
#include "vector_def.h"

namespace lib_gui {
class GuiRenderer {
 public:
  GuiRenderer(lib_core::EngineCore* engine);
  virtual ~GuiRenderer() = default;

  virtual void Draw(lib_core::Vector2 screen_dim) = 0;

 protected:
  lib_core::EngineCore* engine_;
};
}  // namespace lib_gui
