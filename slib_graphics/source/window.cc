#include "window.h"
#include "engine_settings.h"

namespace lib_graphics {
Window::Window() = default;

std::pair<int, int> Window::GetWindowDim() const { return current_dim_; }

std::pair<int, int> Window::GetRenderDim() const {
  std::pair<int, int> win_dim;
  if (g_settings.Fullscreen())
    win_dim = {g_settings.FullscreenWidth(), g_settings.FullscreenHeight()};
  else
    win_dim = {g_settings.WindowedWidth(), g_settings.WindowedHeight()};
  return win_dim;
}
}  // namespace lib_graphics
