#pragma once
#include "entity.h"
#include "unit.h"

namespace lib_graphics {
class Renderer;
}

namespace lib_gui {
class TextSystem;
}

namespace lib_core {
class System : public Unit {
 public:
  System() = default;
  virtual ~System() = default;

  virtual void InitSystem() {}
  virtual void FinalizeSystem() {}

  virtual void LogicUpdate(float dt) {}
  virtual void DrawUpdate(lib_graphics::Renderer* renderer,
                          lib_gui::TextSystem* text_renderer) {}

  bool Initialized() { return initialized_; }
  bool Loaded() { return fully_loaded_; }

  bool IsDrawn() { return draw_; }
  bool IsUpdated() { return update_; }

  void SetDraw(bool draw) { draw_ = draw; }
  void SetUpdate(bool update) { update_ = update; }

 protected:
  template <typename T>
  T& Ts() {
    for (int i = 0; i < sizeof(T); ++i) temporary_memory_.push_back(0);
    return *reinterpret_cast<T*>(
        &temporary_memory_[temporary_memory_.size() - sizeof(T)]);
  }

 private:
  bool initialized_ = false;
  bool fully_loaded_ = false;

  bool draw_ = true;
  bool update_ = true;

  ct::dyn_array<uint8_t> temporary_memory_;

  friend class SystemManager;
};
}  // namespace lib_core
