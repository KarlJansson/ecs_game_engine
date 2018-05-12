#pragma once
#include "entity.h"
#include "entity_manager.h"
#include "vector_def.h"

namespace lib_graphics {
class Mesh;
class Transform;
class Light;
class ParticleEmitter;
}  // namespace lib_graphics
namespace lib_physics {
class Actor;
class Joint;
class Trigger;
}  // namespace lib_physics

namespace lib_core {
class Unit {
 public:
  Unit();
  virtual ~Unit();

  Entity CreateScopedEntity();

  virtual bool Update(float dt);
  virtual void MoveUnit(lib_core::Vector3 dp);
  virtual void RotateUnit(lib_core::Vector3 dr);
  virtual void PositionUnit(lib_core::Vector3 position);

  virtual void Activate();
  virtual void Deactivate();

  bool IsActive();

 protected:
  void PurgeEntities();

  bool active_ = true;
  ct::dyn_array<Entity> scoped_entities_;

  std::unique_ptr<ct::hash_map<lib_core::Entity, lib_graphics::Transform>>
      stored_transforms_;
  std::unique_ptr<ct::hash_map<lib_core::Entity, lib_graphics::Mesh>>
      stored_meshes_;
  std::unique_ptr<ct::hash_map<lib_core::Entity, lib_graphics::Light>>
      stored_lights_;
  std::unique_ptr<ct::hash_map<lib_core::Entity, lib_graphics::ParticleEmitter>>
      stored_emitters_;
  std::unique_ptr<ct::hash_map<lib_core::Entity, lib_physics::Actor>>
      stored_actors_;
  std::unique_ptr<ct::hash_map<lib_core::Entity, lib_physics::Joint>>
      stored_joints_;
  std::unique_ptr<ct::hash_map<lib_core::Entity, lib_physics::Trigger>>
      stored_triggers_;
};
}  // namespace lib_core
