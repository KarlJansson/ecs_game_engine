#pragma once

namespace lib_graphics {
class Skybox {
 public:
  Skybox() = default;
  Skybox(size_t texture) : texture_(texture) {}
  ~Skybox() = default;

  size_t texture_ = 0;
};
}  // namespace lib_graphics
