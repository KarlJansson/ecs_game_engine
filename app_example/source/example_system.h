#pragma once
#include "engine_core.h"
#include "system.h"

namespace app_example {
class ExampleSystem : public lib_core::System {
 public:
  ExampleSystem(lib_core::EngineCore *engine);
  ~ExampleSystem() = default;

  void LogicUpdate(float dt) override;
  void FinalizeSystem() override;

 private:
  lib_core::EngineCore *engine_;
};
}  // namespace app_example
