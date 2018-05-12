#pragma once
#include "engine_core.h"
#include "input_system.h"
#include "window.h"

namespace lib_input {
class InputFactory {
 public:
  InputFactory() = default;
  ~InputFactory() = default;

  std::unique_ptr<InputSystem> CreateInputSystem(lib_core::EngineCore* engine);
};
}  // namespace lib_input
