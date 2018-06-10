#pragma once
#include "gui_rect.h"
#include "system.h"

namespace lib_gui {
class RectSystem : public lib_core::System {
 public:
  RectSystem() = default;
  ~RectSystem() override = default;

  void LogicUpdate(float dt) override;
  virtual void PurgeGpuResources() = 0;

 private:
};
}  // namespace lib_gui
