#include "example_system.h"
#include "actor.h"
#include "factory.hpp"
#include "fps_camera_system.h"
#include "mesh.h"
#include "scripted_system.h"
#include "toggle_input.h"
#include "transform.h"

using namespace lib_graphics;
using namespace lib_physics;
using namespace lib_core;

namespace app_example {
ExampleSystem::ExampleSystem(lib_core::EngineCore* engine) : engine_(engine) {
  // Create a standard engine fps camera
  issue_command(AddSystemCommand(std::make_unique<FpsCameraSystem>(engine), 1));
  issue_command(FpsCameraSystem::SetStartLocation({0.f, 10.f, 0.f}));
  issue_command(FpsCameraSystem::SetCameraExposure(1.f));

  // Bind an exit binding to Escape
  lib_input::ToggleInput ti;
  ti.function_binding = [&](float dt) { engine_->GetWindow()->CloseWindow(); };
  ti.key_combos.push_back({lib_input::kEscape});
  g_ent_mgr.AddComponent(CreateScopedEntity(), ti);

  // Create a scripted system to ease placement of objects
  issue_command(AddSystemCommand(
      std::make_unique<lib_core::ScriptedSystem>(
          engine, "./content/example_script.txt", lib_core::Vector3(0.f)),
      1));
}

void ExampleSystem::LogicUpdate(float dt) {}

void ExampleSystem::FinalizeSystem() {}
}  // namespace app_example
