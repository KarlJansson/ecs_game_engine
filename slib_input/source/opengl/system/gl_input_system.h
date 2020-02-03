#pragma once
#include "engine_core.h"
#include "input_system.h"
#include "window.h"

namespace lib_input {
class GlInputSystem : public InputSystem {
 public:
  GlInputSystem(lib_core::EngineCore* engine);
  ~GlInputSystem() override = default;

  void InitSystem() override;
  void LogicUpdate(float dt) override;

  bool KeyPressed(Key key) override;
  bool KeyReleased(Key key) override;

  bool MousePressed(Key key) override;
  bool MouseReleased(Key key) override;

  bool ButtonPressed(int controller, PadButton button) override;
  bool ButtonReleased(int controller, PadButton button) override;

 protected:
  int ConvertKey(Key k) override;
  int ConvertButton(PadButton b) override;
  int ConvertStick(PadStick s) override;

 private:
  lib_core::EngineCore* engine_;
  struct GLFWwindow* window_;

  double x_pos_, y_pos_;
  double last_x_pos_, last_y_pos_;
  bool first_update_;
};
}  // namespace lib_input
