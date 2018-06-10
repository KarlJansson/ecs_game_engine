#pragma once
#include "engine_core.h"
#include "graphics_commands.h"
#include "joint.h"
#include "system.h"
#include "trigger.h"

namespace lib_core {
class ScriptedSystem : public lib_core::System {
 public:
  ScriptedSystem(lib_core::EngineCore *engine, ct::string script_path,
                 lib_core::Vector3 offset);
  ~ScriptedSystem() override = default;

  void InitSystem() override;
  void LogicUpdate(float dt) override;
  void FinalizeSystem() override;
  void Activate() override;
  void Deactivate() override;

  virtual void PurgeSystem();

  void AddType(std::string name, std::function<void(void)> func);

 protected:
  void ParseScript();

  void ParseForLoop();
  void ParseActorMesh();
  void ParseActorJoint();
  void ParseParticleEmitter();

  size_t MaterialId();
  lib_graphics::Material ParseMaterial();

  ct::hash_map<size_t, size_t> material_map_;
  ct::dyn_array<std::unique_ptr<lib_core::Unit>> units_;

  float check_timer_ = 1.f;
  size_t time_stamp_;
  float mod_;

  ct::hash_map<ct::string, std::function<void(void)>> valid_types_;

  ct::hash_map<ct::string, lib_core::Entity> actor_map_;
  ct::hash_map<ct::string, ct::string> variable_map_;

  size_t script_cursor_;
  lib_core::EngineCore *engine_;
  ct::string script_path_;

  lib_core::Vector3 offset_;
  ct::dyn_array<char> script_buffer_;
};
}  // namespace lib_core
