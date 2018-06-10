#pragma once
#include "input_commands.h"
#include "key_definitions.h"
#include "system.h"
#include "vector_def.h"

namespace lib_input {
class InputSystem : public lib_core::System {
 public:
  InputSystem() = default;
  ~InputSystem() override = default;

  void LogicUpdate(float dt) override;
  void SetCursor(bool enable);

  virtual bool KeyPressed(Key key) = 0;
  virtual bool KeyReleased(Key key) = 0;

  virtual bool MousePressed(Key key) = 0;
  virtual bool MouseReleased(Key key) = 0;

  virtual bool ButtonPressed(int controller, PadButton button) = 0;
  virtual bool ButtonReleased(int controller, PadButton button) = 0;

  lib_core::Vector2 MousePos();
  lib_core::Vector2 MouseDelta();

  float StickPos(int controller, PadStick stick);

  int PresentContorllerId(int offset = 0);

 protected:
  lib_core::Vector2 delta_ = {.0f, .0f};
  lib_core::Vector2 pos_ = {.0f, .0f};
  lib_core::Vector2 mouse_speed_ = {.0f, .0f};

  std::atomic<bool> cursor_disabled_ = {true};

  ct::dyn_array<int> present_sticks_;
  ct::dyn_array<float> stick_pos_[16];
};
}  // namespace lib_input
