#pragma once

namespace lib_graphics {
class Renderer {
 public:
  Renderer() = default;
  virtual ~Renderer() = default;

  virtual void InitRenderer() = 0;
  virtual void RenderFrame(float dt) = 0;

  virtual void Clear(lib_core::Vector4 color) = 0;
};
}  // namespace lib_graphics
