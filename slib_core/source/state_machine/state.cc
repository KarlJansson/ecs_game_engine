#include "state.h"

namespace lib_core {
void State::OnEnter() {}
void State::OnExit() {}
bool State::Update(float dt) { return false; }
}  // namespace lib_core
