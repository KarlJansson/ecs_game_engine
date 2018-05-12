#include "input_system.h"
#include "contiguous_input.h"
#include "engine_settings.h"
#include "entity_manager.h"
#include "toggle_input.h"

namespace lib_input {
void InputSystem::LogicUpdate(float dt) {
  auto contig_comps = g_ent_mgr.GetNewCbt<ContiguousInput>();
  if (contig_comps) {
    auto c_old = g_ent_mgr.GetOldCbt<ContiguousInput>()->data();
    for (auto& c : *contig_comps) {
      c.active = c_old->active;

      if (c.active) {
        bool condition = true;
        for (auto& key_vec : c.key_combos) {
          condition = true;
          for (auto k : key_vec) {
            if (KeyReleased(k)) {
              condition = false;
              break;
            }
          }
          if (condition) break;
        }

        if (condition) c.function_binding(dt);
      }
      c_old++;
    }
  }

  auto toggle_comps = g_ent_mgr.GetNewCbt<ToggleInput>();
  if (toggle_comps) {
    auto c_old = g_ent_mgr.GetOldCbt<ToggleInput>()->data();
    for (auto& c : *toggle_comps) {
      c.active = c_old->active;

      if (c.active) {
        bool condition = true;
        for (auto& key_vec : c.key_combos) {
          condition = true;
          for (auto k : key_vec) {
            if (KeyReleased(k)) {
              condition = false;
              break;
            }
          }
          if (condition) break;
        }

        if (condition) {
          if (!c_old->toggled) c.function_binding(dt);
          c.toggled = true;
        } else {
          c_old->toggled = false;
        }
      }
      c_old++;
    }
  }
}

void InputSystem::SetCursor(bool enable) { cursor_disabled_ = !enable; }

lib_core::Vector2 InputSystem::MousePos() { return pos_; }

lib_core::Vector2 InputSystem::MouseDelta() {
  if (mouse_speed_.Zero()) return mouse_speed_;

  auto sensitivity = g_settings.MouseSensitivity();
  return {mouse_speed_[0] * sensitivity, mouse_speed_[1] * sensitivity};
}

lib_core::Vector2 InputSystem::StickPos(int controller, int stick) {
  if (controller < 16 && controller > -1 && stick > -1 &&
      stick < stick_pos_[controller].size()) {
    if (std::abs(stick_pos_[controller][stick][0]) > 0.3f ||
        std::abs(stick_pos_[controller][stick][1]) > 0.3f)
      return stick_pos_[controller][stick];
  }
  return {0.f};
}

int InputSystem::PresentContorllerId(int offset) {
  if (present_sticks_.size() <= offset) return -1;
  return present_sticks_[offset];
}
}  // namespace lib_input
