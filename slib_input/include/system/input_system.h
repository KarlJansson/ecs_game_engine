#pragma once
#include "input_commands.h"
#include "system.h"
#include "vector_def.h"

namespace lib_input {
class InputSystem : public lib_core::System {
 public:
  InputSystem() = default;
  virtual ~InputSystem() = default;

  void LogicUpdate(float dt);
  void SetCursor(bool enable);

  virtual bool KeyPressed(int key) = 0;
  virtual bool KeyReleased(int key) = 0;

  virtual bool MousePressed(int key) = 0;
  virtual bool MouseReleased(int key) = 0;

  virtual bool ButtonPressed(int stick, int button) = 0;
  virtual bool ButtonReleased(int stick, int button) = 0;

  lib_core::Vector2 MousePos();
  lib_core::Vector2 MouseDelta();

  lib_core::Vector2 StickPos(int controller, int stick = 0);

  int PresentContorllerId(int offset = 0);

 protected:
  lib_core::Vector2 delta_ = {.0f, .0f};
  lib_core::Vector2 pos_ = {.0f, .0f};
  lib_core::Vector2 mouse_speed_ = {.0f, .0f};

  std::atomic<bool> cursor_disabled_ = {true};

  ct::dyn_array<int> present_sticks_;
  ct::dyn_array<lib_core::Vector2> stick_pos_[16];
};
}  // namespace lib_input
