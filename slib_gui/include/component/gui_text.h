#pragma once
#include "core_utilities.h"
#include "vector_def.h"

namespace lib_gui {
class GuiText {
 public:
  enum HAlignment { kLeft, kRight, kCenter };

  GuiText() = default;
  GuiText(ct::string t, lib_core::Vector2 pos, lib_core::Vector2 hs,
          lib_core::Vector4 col, size_t fnt = 0, HAlignment h_align = kLeft)
      : font(fnt),
        position(pos),
        half_size(hs),
        rgba(col),
        text(t),
        h_alignment(h_align) {}
  ~GuiText() = default;

  size_t font = 0;
  lib_core::Vector2 position;
  lib_core::Vector2 half_size;
  lib_core::Vector4 rgba;
  ct::string text;
  HAlignment h_alignment;

  uint8_t layer = 0;
};
}  // namespace lib_gui
