#pragma once
#include "engine_settings.h"
#include "window.h"

namespace lib_graphics {
class VlWindow : public Window {
 public:
  VlWindow();
  ~VlWindow() override;

  void CloseWindow() override;
  void SwapBuffers() override;
  int ShouldClose() override;
  void SetRenderContext() override;
  void SetLoadContext() override;
  [[nodiscard]] void *GetWindowHandle() const override;
  [[nodiscard]] bool NeedsRestart() const override;
  void Rebuild() override;
  bool CheckCapabilities() override;

 private:
  void CreateRenderWindow();
  std::string VersionString(uint32_t version_bitmask);

  bool fullscreen_setting_;
  bool windowed_setting_;

  bool render_claimed_ = false;
  bool load_claimed_ = false;

  void *window_ = nullptr;
  void *load_context_ = nullptr;
};
}  // namespace lib_graphics
