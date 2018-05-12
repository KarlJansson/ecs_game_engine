#pragma once
#include <utility>
#include "vector_def.h"

namespace lib_gui {
class GuiRect {
 public:
  GuiRect(lib_core::Vector2 pos, lib_core::Vector2 hs, lib_core::Vector4 col,
          uint8_t l = 0)
      : position_(std::move(pos)),
        half_size_(std::move(hs)),
        rgba_(std::move(col)),
        layer(std::move(l)) {}
  GuiRect() = default;
  ~GuiRect() = default;

  lib_core::Vector2 position_;
  lib_core::Vector2 half_size_;
  lib_core::Vector4 rgba_;

  uint8_t layer = 0;
};
}  // namespace lib_gui
