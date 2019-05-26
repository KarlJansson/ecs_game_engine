#pragma once
#include "engine_settings.h"

namespace lib_graphics {
class Window {
public:
  struct GpuCapabilities {
    size_t max_fragment_uniforms;
    size_t max_vertex_uniforms;
    size_t max_geometry_uniforms;

    float version;
  };

  Window();
  virtual ~Window() = default;

  virtual void CloseWindow() = 0;
  virtual void SwapBuffers() = 0;
  virtual int ShouldClose() = 0;
  virtual void SetRenderContext() = 0;
  virtual void SetLoadContext() = 0;
  [[nodiscard]] virtual void *GetWindowHandle() const = 0;
  [[nodiscard]] virtual bool NeedsRestart() const = 0;
  virtual void Rebuild() = 0;
  virtual bool CheckCapabilities() = 0;

  [[nodiscard]] std::pair<int, int> GetWindowDim() const;
  [[nodiscard]] std::pair<int, int> GetRenderDim() const;
  const GpuCapabilities &Capabilities() { return gpu_capabilities_; }

protected:
  std::pair<int, int> current_dim_;
  GpuCapabilities gpu_capabilities_;
};
} // namespace lib_graphics
