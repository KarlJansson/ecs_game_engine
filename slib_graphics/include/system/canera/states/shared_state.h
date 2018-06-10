#pragma once
#include "state.h"

namespace lib_graphics {
class SharedState : public lib_core::State {
 public:
  SharedState() = default;
  ~SharedState() override = default;

  void OnEnter() override;
  void OnExit() override;
  bool Update(float dt) override;

 protected:
 private:
};
}  // namespace app_fract_editor
