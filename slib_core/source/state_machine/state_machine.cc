#include "state_machine.h"

namespace lib_core {
StateMachine::StateMachine(MachineType type) : type_(type) {}

void StateMachine::ReplaceTopState(std::unique_ptr<State> state) {
  state_stack_.back()->OnExit();
  state->owner_ = this;
  state->OnEnter();
  state_stack_[state_stack_.size() - 1] = std::move(state);
}

void StateMachine::PushState(std::unique_ptr<State> state) {
  if (!state_stack_.empty() && type_ == kSingleState)
    state_stack_.back()->OnExit();
  state->owner_ = this;
  state->OnEnter();
  state_stack_.emplace_back(std::move(state));
}

void StateMachine::PopState() {
  state_stack_.back()->OnExit();
  state_stack_.pop_back();
  if (!state_stack_.empty()) state_stack_.back()->OnEnter();
}

void StateMachine::Update(float dt) {
  if (state_stack_.empty()) return;
  switch (type_) {
    case kSingleState:
      state_stack_.back()->Update(dt);
      break;
    case kMultiState:
      for (int i = int(state_stack_.size()) - 1; i >= 0; --i)
        if (i < state_stack_.size()) state_stack_[i]->Update(dt);
      break;
  }
}

size_t StateMachine::GetNrStates() { return state_stack_.size(); }
}  // namespace lib_core
