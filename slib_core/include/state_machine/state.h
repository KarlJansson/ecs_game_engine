#pragma once
#include "unit.h"

namespace lib_core {
class State : public Unit {
 public:
  State() = default;
  ~State() override = default;

  virtual void OnEnter();
  virtual void OnExit();
  bool Update(float dt) override;

 protected:
  class StateMachine* owner_;

  friend class StateMachine;
};
}  // namespace lib_core
