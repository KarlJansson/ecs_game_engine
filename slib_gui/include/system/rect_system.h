#pragma once
#include "engine_core.h"
#include "gui_rect.h"
#include "system.h"

namespace lib_gui {
class RectSystem : public lib_core::System {
 public:
  RectSystem(lib_core::EngineCore* engine);
  ~RectSystem() override = default;

  void LogicUpdate(float dt) override;
  virtual void PurgeGpuResources() = 0;

 protected:
  lib_core::EngineCore* engine_;
};
}  // namespace lib_gui
