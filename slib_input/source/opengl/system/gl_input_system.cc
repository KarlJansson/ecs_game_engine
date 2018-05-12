#ifdef WindowsBuild
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WIN32_WINDOWS 0x0601
#define NOMINMAX
#include <windows.h>
#endif

#include <GLFW/glfw3.h>
#include "engine_settings.h"
#include "gl_input_system.h"
#include "key_definitions.h"

namespace lib_input {
GlInputSystem::GlInputSystem(lib_core::EngineCore *engine)
    : engine_(engine), window_(nullptr) {}

void GlInputSystem::InitSystem() {
  first_update_ = true;
  window_ = static_cast<GLFWwindow *>(engine_->GetWindow()->GetWindowHandle());
  glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void GlInputSystem::LogicUpdate(float dt) {
  if (!window_) return;

  glfwPollEvents();
  auto cursor_mode = glfwGetInputMode(window_, GLFW_CURSOR);
  if (cursor_mode == GLFW_CURSOR_DISABLED && !cursor_disabled_)
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  else if (cursor_mode == GLFW_CURSOR_NORMAL && cursor_disabled_)
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  last_x_pos_ = x_pos_, last_y_pos_ = y_pos_;
  glfwGetCursorPos(window_, &x_pos_, &y_pos_);
  pos_ = {float(x_pos_), float(y_pos_)};

  if (!first_update_) {
    if (cursor_disabled_)
      delta_ = {float(x_pos_ - last_x_pos_), float(last_y_pos_ - y_pos_)};
    else
      delta_ = {0.f};

    mouse_speed_ =
        co::lerp(mouse_speed_, delta_, 1.f / g_settings.MouseSmoothing());
    if (mouse_speed_.Length() < 1.e-6f) mouse_speed_.ZeroMem();
  } else
    first_update_ = false;

  // Check controller input
  present_sticks_.clear();
  for (int i = 0; i < 16; ++i) {
    stick_pos_[i].clear();
    int present = glfwJoystickPresent(GLFW_JOYSTICK_1 + i);
    if (present) {
      ct::string joy_name = glfwGetJoystickName(GLFW_JOYSTICK_1 + i);
      int count;
      const float *axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1 + i, &count);
      for (int ii = 0; ii < count; ii += 2)
        stick_pos_[i].push_back({axes[ii], axes[ii + 1]});

      present_sticks_.push_back(i);
    }
  }

  InputSystem::LogicUpdate(dt);
}

bool GlInputSystem::KeyPressed(int key) {
  if (!window_) return false;
  return glfwGetKey(window_, key) > 0;
}

bool GlInputSystem::KeyReleased(int key) { return !KeyPressed(key); }

bool GlInputSystem::MousePressed(int key) {
  if (!window_) return false;
  return glfwGetMouseButton(window_, key) > 0;
}

bool GlInputSystem::MouseReleased(int key) { return !MousePressed(key); }

bool GlInputSystem::ButtonPressed(int stick, int button) {
  int count;
  const unsigned char *axes =
      glfwGetJoystickButtons(GLFW_JOYSTICK_1 + stick, &count);
  if (axes && button > -1 && button < count) return axes[button] > 0;
  return false;
}

bool GlInputSystem::ButtonReleased(int stick, int button) {
  return !ButtonPressed(stick, button);
}
}  // namespace lib_input
