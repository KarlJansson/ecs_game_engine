#pragma once
#include "system_manager.h"

namespace lib_core {
class AddSystemCommand : public Command {
 public:
  enum DefaultPriorities { Prerender = 4000, Postrun = 3000, Prerun = 2000 };

  AddSystemCommand() = default;
  AddSystemCommand(std::shared_ptr<lib_core::System> system, size_t priority)
      : system(system), priority(priority) {
    base_id = g_sys_mgr.GenerateResourceIds(1);
  }

  size_t SystemId() { return base_id; }

  std::shared_ptr<lib_core::System> system;
  size_t priority;
};

class RemoveSystemCommand : public Command {
 public:
  RemoveSystemCommand() = default;
  RemoveSystemCommand(size_t system_id) : system_id(system_id) {}
  size_t system_id;
};
}  // namespace lib_core
