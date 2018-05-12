#pragma once
#include "engine_core.h"
#include "input_system.h"
#include "window.h"

namespace lib_input {
class GlInputSystem : public InputSystem {
 public:
  GlInputSystem(lib_core::EngineCore* engine);
  ~GlInputSystem() = default;

  void InitSystem() override;
  void LogicUpdate(float dt) override;

  bool KeyPressed(int key) override;
  bool KeyReleased(int key) override;

  bool MousePressed(int key) override;
  bool MouseReleased(int key) override;

  bool ButtonPressed(int stick, int button) override;
  bool ButtonReleased(int stick, int button) override;

 private:
  lib_core::EngineCore* engine_;
  struct GLFWwindow* window_;

  double x_pos_, y_pos_;
  double last_x_pos_, last_y_pos_;
  bool first_update_;
};
}  // namespace lib_input
