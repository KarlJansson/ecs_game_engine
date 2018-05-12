#pragma once
#include "state.h"

namespace lib_core {
class StateMachine {
 public:
  enum MachineType { kSingleState, kMultiState };

  StateMachine(MachineType type);
  ~StateMachine() = default;

  void ReplaceTopState(std::unique_ptr<State> state);
  void PushState(std::unique_ptr<State> state);
  void PopState();

  void Update(float dt);

  size_t GetNrStates();

 private:
  MachineType type_;
  ct::dyn_array<std::unique_ptr<State>> state_stack_;
};
}  // namespace lib_core
