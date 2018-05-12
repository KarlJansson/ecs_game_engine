#pragma once
#include "unit.h"

namespace lib_core {
class State : public Unit {
 public:
  State() = default;
  virtual ~State() = default;

  virtual void OnEnter();
  virtual void OnExit();
  virtual bool Update(float dt);

 protected:
  class StateMachine* owner_;

  friend class StateMachine;
};
}  // namespace lib_core
