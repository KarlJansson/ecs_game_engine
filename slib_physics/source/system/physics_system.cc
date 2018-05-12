#include "physics_system.h"
#include "actor_handler.h"
#include "physics_commands.h"
#include "system_manager.h"

namespace lib_physics {
void PhysicsSystem::SetTimeMultiplier(float multiplier) {
  time_multiplier_ = multiplier;
}

float PhysicsSystem::TimeMultiplier() { return time_multiplier_; }

void PhysicsSystem::BaseUpdate() {
  auto add_commands = g_sys_mgr.GetCommands<AddMeshSourceCommmand>();
  if (add_commands && !add_commands->empty()) {
    for (auto& command : *add_commands)
      GetActorHandler().mesh_sources_[command.mesh_id] =
          std::move(command.physic_data);
    add_commands->clear();
  }

  auto remove_commands = g_sys_mgr.GetCommands<RemoveMeshSourceCommmand>();
  if (remove_commands && !remove_commands->empty()) {
    for (auto& command : *remove_commands)
      GetActorHandler().mesh_sources_.erase(command.mesh_id);
    remove_commands->clear();
  }
}
}  // namespace lib_physics
