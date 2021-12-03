#ifdef WindowsBuild
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WIN32_WINDOWS 0x0601
#define NOMINMAX
#include <windows.h>
#endif

#include <GLFW/glfw3.h>
#include "core_utilities.h"
#include "cursor_input.h"
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

  auto win_dim = engine_->GetWindow()->GetWindowDim();
  auto cursor_comp = g_ent_mgr.GetNewCbeW<lib_input::CursorInput>();
  auto a_ratio = float(win_dim.second) / float(win_dim.first);
  cursor_comp->pos = {
      float((co::clamp(0, win_dim.first, int(x_pos_)) / float(win_dim.first)) /
            a_ratio),
      float(1.f -
            co::clamp(0, win_dim.second, int(y_pos_)) / float(win_dim.second))};

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
      for (int ii = 0; ii < count; ++ii) stick_pos_[i].push_back(axes[ii]);
      present_sticks_.push_back(i);
    }
  }

  InputSystem::LogicUpdate(dt);
}

bool GlInputSystem::KeyPressed(Key key) {
  if (!window_) return false;
  switch (key) {
    case Key::kMouseLeft:
    case Key::kMouseRight:
    case Key::kMouseMiddle:
    case Key::kMouse4:
    case Key::kMouse5:
    case Key::kMouse6:
    case Key::kMouse7:
    case Key::kMouse8:
      return MousePressed(key);
    default:
      return glfwGetKey(window_, ConvertKey(key)) > 0;
  };
  return glfwGetKey(window_, ConvertKey(key)) > 0;
}

bool GlInputSystem::KeyReleased(Key key) { return !KeyPressed(key); }

bool GlInputSystem::MousePressed(Key key) {
  if (!window_) return false;
  return glfwGetMouseButton(window_, ConvertKey(key)) > 0;
}

bool GlInputSystem::MouseReleased(Key key) { return !MousePressed(key); }

bool GlInputSystem::ButtonPressed(int stick, PadButton button) {
  int count;
  auto casted_button = ConvertButton(button);
  const unsigned char *axes =
      glfwGetJoystickButtons(GLFW_JOYSTICK_1 + stick, &count);
  if (axes && casted_button > -1 && casted_button < count)
    return axes[casted_button] > 0;
  return false;
}

bool GlInputSystem::ButtonReleased(int stick, PadButton button) {
  return !ButtonPressed(stick, button);
}

int GlInputSystem::ConvertKey(Key k) {
  switch (k) {
    case Key::kMouseLeft:
      return GLFW_MOUSE_BUTTON_1;
    case Key::kMouseRight:
      return GLFW_MOUSE_BUTTON_2;
    case Key::kMouseMiddle:
      return GLFW_MOUSE_BUTTON_3;
    case Key::kMouse4:
      return GLFW_MOUSE_BUTTON_4;
    case Key::kMouse5:
      return GLFW_MOUSE_BUTTON_5;
    case Key::kMouse6:
      return GLFW_MOUSE_BUTTON_6;
    case Key::kMouse7:
      return GLFW_MOUSE_BUTTON_7;
    case Key::kMouse8:
      return GLFW_MOUSE_BUTTON_8;
    case Key::kSpace:
      return GLFW_KEY_SPACE;
    case Key::kApostrophe:
      return GLFW_KEY_APOSTROPHE;
    case Key::kComma:
      return GLFW_KEY_COMMA;
    case Key::kMinus:
      return GLFW_KEY_MINUS;
    case Key::kPeriod:
      return GLFW_KEY_PERIOD;
    case Key::kSlash:
      return GLFW_KEY_SLASH;
    case Key::k0:
      return GLFW_KEY_0;
    case Key::k1:
      return GLFW_KEY_1;
    case Key::k2:
      return GLFW_KEY_2;
    case Key::k3:
      return GLFW_KEY_3;
    case Key::k4:
      return GLFW_KEY_4;
    case Key::k5:
      return GLFW_KEY_5;
    case Key::k6:
      return GLFW_KEY_6;
    case Key::k7:
      return GLFW_KEY_7;
    case Key::k8:
      return GLFW_KEY_8;
    case Key::k9:
      return GLFW_KEY_9;
    case Key::kSemicolon:
      return GLFW_KEY_SEMICOLON;
    case Key::kEqual:
      return GLFW_KEY_EQUAL;
    case Key::kA:
      return GLFW_KEY_A;
    case Key::kB:
      return GLFW_KEY_B;
    case Key::kC:
      return GLFW_KEY_C;
    case Key::kD:
      return GLFW_KEY_D;
    case Key::kE:
      return GLFW_KEY_E;
    case Key::kF:
      return GLFW_KEY_F;
    case Key::kG:
      return GLFW_KEY_G;
    case Key::kH:
      return GLFW_KEY_H;
    case Key::kI:
      return GLFW_KEY_I;
    case Key::kJ:
      return GLFW_KEY_J;
    case Key::kK:
      return GLFW_KEY_K;
    case Key::kL:
      return GLFW_KEY_L;
    case Key::kM:
      return GLFW_KEY_M;
    case Key::kN:
      return GLFW_KEY_N;
    case Key::kO:
      return GLFW_KEY_O;
    case Key::kP:
      return GLFW_KEY_P;
    case Key::kQ:
      return GLFW_KEY_Q;
    case Key::kR:
      return GLFW_KEY_R;
    case Key::kS:
      return GLFW_KEY_S;
    case Key::kT:
      return GLFW_KEY_T;
    case Key::kU:
      return GLFW_KEY_U;
    case Key::kV:
      return GLFW_KEY_V;
    case Key::kW:
      return GLFW_KEY_W;
    case Key::kX:
      return GLFW_KEY_X;
    case Key::kY:
      return GLFW_KEY_Y;
    case Key::kZ:
      return GLFW_KEY_Z;
    case Key::kLeftBracket:
      return GLFW_KEY_LEFT_BRACKET;
    case Key::kBackslash:
      return GLFW_KEY_BACKSLASH;
    case Key::kRightBracket:
      return GLFW_KEY_RIGHT_BRACKET;
    case Key::kGraveAccent:
      return GLFW_KEY_GRAVE_ACCENT;
    case Key::kWorld1:
      return GLFW_KEY_WORLD_1;
    case Key::kWorld2:
      return GLFW_KEY_WORLD_2;
    case Key::kEscape:
      return GLFW_KEY_ESCAPE;
    case Key::kEnter:
      return GLFW_KEY_ENTER;
    case Key::kTab:
      return GLFW_KEY_TAB;
    case Key::kBackspace:
      return GLFW_KEY_BACKSPACE;
    case Key::kInsert:
      return GLFW_KEY_INSERT;
    case Key::kDelete:
      return GLFW_KEY_DELETE;
    case Key::kRight:
      return GLFW_KEY_RIGHT;
    case Key::kLeft:
      return GLFW_KEY_LEFT;
    case Key::kDown:
      return GLFW_KEY_DOWN;
    case Key::kUp:
      return GLFW_KEY_UP;
    case Key::kPageUp:
      return GLFW_KEY_PAGE_UP;
    case Key::kPageDown:
      return GLFW_KEY_PAGE_DOWN;
    case Key::kHome:
      return GLFW_KEY_HOME;
    case Key::kEnd:
      return GLFW_KEY_END;
    case Key::kCapsLock:
      return GLFW_KEY_CAPS_LOCK;
    case Key::kScrollLock:
      return GLFW_KEY_SCROLL_LOCK;
    case Key::kNumLock:
      return GLFW_KEY_NUM_LOCK;
    case Key::kPrintScreen:
      return GLFW_KEY_PRINT_SCREEN;
    case Key::kPause:
      return GLFW_KEY_PAUSE;
    case Key::kF1:
      return GLFW_KEY_F1;
    case Key::kF2:
      return GLFW_KEY_F2;
    case Key::kF3:
      return GLFW_KEY_F3;
    case Key::kF4:
      return GLFW_KEY_F4;
    case Key::kF5:
      return GLFW_KEY_F5;
    case Key::kF6:
      return GLFW_KEY_F6;
    case Key::kF7:
      return GLFW_KEY_F7;
    case Key::kF8:
      return GLFW_KEY_F8;
    case Key::kF9:
      return GLFW_KEY_F9;
    case Key::kF10:
      return GLFW_KEY_F10;
    case Key::kF11:
      return GLFW_KEY_F11;
    case Key::kF12:
      return GLFW_KEY_F12;
    case Key::kF13:
      return GLFW_KEY_F13;
    case Key::kF14:
      return GLFW_KEY_F14;
    case Key::kF15:
      return GLFW_KEY_F15;
    case Key::kF16:
      return GLFW_KEY_F16;
    case Key::kF17:
      return GLFW_KEY_F17;
    case Key::kF18:
      return GLFW_KEY_F18;
    case Key::kF19:
      return GLFW_KEY_F19;
    case Key::kF20:
      return GLFW_KEY_F20;
    case Key::kF21:
      return GLFW_KEY_F21;
    case Key::kF22:
      return GLFW_KEY_F22;
    case Key::kF23:
      return GLFW_KEY_F23;
    case Key::kF24:
      return GLFW_KEY_F24;
    case Key::kF25:
      return GLFW_KEY_F25;
    case Key::kKp0:
      return GLFW_KEY_KP_0;
    case Key::kKp1:
      return GLFW_KEY_KP_1;
    case Key::kKp2:
      return GLFW_KEY_KP_2;
    case Key::kKp3:
      return GLFW_KEY_KP_3;
    case Key::kKp4:
      return GLFW_KEY_KP_4;
    case Key::kKp5:
      return GLFW_KEY_KP_5;
    case Key::kKp6:
      return GLFW_KEY_KP_6;
    case Key::kKp7:
      return GLFW_KEY_KP_7;
    case Key::kKp8:
      return GLFW_KEY_KP_8;
    case Key::kKp9:
      return GLFW_KEY_KP_9;
    case Key::kKpDecimal:
      return GLFW_KEY_KP_DECIMAL;
    case Key::kKpDivide:
      return GLFW_KEY_KP_DIVIDE;
    case Key::kKpMultiply:
      return GLFW_KEY_KP_MULTIPLY;
    case Key::kKpSubtract:
      return GLFW_KEY_KP_SUBTRACT;
    case Key::kKpAdd:
      return GLFW_KEY_KP_ADD;
    case Key::kKpEnter:
      return GLFW_KEY_KP_ENTER;
    case Key::kKpEqual:
      return GLFW_KEY_KP_EQUAL;
    case Key::kLeftShift:
      return GLFW_KEY_LEFT_SHIFT;
    case Key::kLeftControl:
      return GLFW_KEY_LEFT_CONTROL;
    case Key::kLeftAlt:
      return GLFW_KEY_LEFT_ALT;
    case Key::kLeftSuper:
      return GLFW_KEY_LEFT_SUPER;
    case Key::kRightShift:
      return GLFW_KEY_RIGHT_SHIFT;
    case Key::kRightControl:
      return GLFW_KEY_RIGHT_CONTROL;
    case Key::kRightAlt:
      return GLFW_KEY_RIGHT_ALT;
    case Key::kRightSuper:
      return GLFW_KEY_RIGHT_SUPER;
    case Key::kMenu:
      return GLFW_KEY_MENU;
  }
  return -1;
}

int GlInputSystem::ConvertButton(PadButton b) {
  switch (b) {
    case PadButton::kButtonA:
      return GLFW_GAMEPAD_BUTTON_A;
    case PadButton::kButtonB:
      return GLFW_GAMEPAD_BUTTON_B;
    case PadButton::kButtonX:
      return GLFW_GAMEPAD_BUTTON_X;
    case PadButton::kButtonY:
      return GLFW_GAMEPAD_BUTTON_Y;
    case PadButton::kShoulderL:
      return GLFW_GAMEPAD_BUTTON_LEFT_BUMPER;
    case PadButton::kShoulderR:
      return GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER;
    case PadButton::kButtonBack:
      return GLFW_GAMEPAD_BUTTON_BACK;
    case PadButton::kButtonStart:
      return GLFW_GAMEPAD_BUTTON_START;
    case PadButton::kButtonGuide:
      return GLFW_GAMEPAD_BUTTON_GUIDE;
    case PadButton::kStickL:
      return GLFW_GAMEPAD_BUTTON_LEFT_THUMB;
    case PadButton::kStickR:
      return GLFW_GAMEPAD_BUTTON_RIGHT_THUMB;
    case PadButton::kDigitalUp:
      return GLFW_GAMEPAD_BUTTON_DPAD_UP;
    case PadButton::kDigitalRight:
      return GLFW_GAMEPAD_BUTTON_DPAD_RIGHT;
    case PadButton::kDigitalDown:
      return GLFW_GAMEPAD_BUTTON_DPAD_DOWN;
    case PadButton::kDigitalLeft:
      return GLFW_GAMEPAD_BUTTON_DPAD_LEFT;
  }
  return -1;
}

int GlInputSystem::ConvertStick(PadStick s) {
  switch (s) {
    case PadStick::kStickLx:
      return GLFW_GAMEPAD_AXIS_LEFT_X;
    case PadStick::kStickLy:
      return GLFW_GAMEPAD_AXIS_LEFT_Y;
    case PadStick::kTriggerL:
      return GLFW_GAMEPAD_AXIS_LEFT_TRIGGER;
    case PadStick::kStickRx:
      return GLFW_GAMEPAD_AXIS_RIGHT_X;
    case PadStick::kStickRy:
      return GLFW_GAMEPAD_AXIS_RIGHT_Y;
    case PadStick::kTriggerR:
      return GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER;
  }
  return -1;
}

}  // namespace lib_input
