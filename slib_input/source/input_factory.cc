#include "input_factory.h"
#include "gl_input_system.h"

namespace lib_input {
std::unique_ptr<InputSystem> InputFactory::CreateInputSystem(
    lib_core::EngineCore* engine) {
  return std::make_unique<GlInputSystem>(engine);
}
}  // namespace lib_input
