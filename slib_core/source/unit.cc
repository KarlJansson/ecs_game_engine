#include "unit.h"
#include "actor.h"
#include "joint.h"
#include "light.h"
#include "mesh.h"
#include "particle_emitter.h"
#include "transform.h"
#include "trigger.h"

namespace lib_core {
Unit::Unit() {
  stored_transforms_ = std::make_unique<
      ct::hash_map<lib_core::Entity, lib_graphics::Transform>>();
  stored_meshes_ =
      std::make_unique<ct::hash_map<lib_core::Entity, lib_graphics::Mesh>>();
  stored_lights_ =
      std::make_unique<ct::hash_map<lib_core::Entity, lib_graphics::Light>>();
  stored_emitters_ = std::make_unique<
      ct::hash_map<lib_core::Entity, lib_graphics::ParticleEmitter>>();
  stored_actors_ =
      std::make_unique<ct::hash_map<lib_core::Entity, lib_physics::Actor>>();
  stored_joints_ =
      std::make_unique<ct::hash_map<lib_core::Entity, lib_physics::Joint>>();
  stored_triggers_ =
      std::make_unique<ct::hash_map<lib_core::Entity, lib_physics::Trigger>>();
}

Unit::~Unit() { PurgeEntities(); }

Entity Unit::CreateScopedEntity() {
  auto ent = g_ent_mgr.CreateEntity();
  scoped_entities_.push_back(ent);
  return ent;
}

bool Unit::Update(float dt) { return false; }

void Unit::MoveUnit(lib_core::Vector3 dp) {
  for (auto& e : scoped_entities_) {
    auto actor_w = g_ent_mgr.GetNewCbeW<lib_physics::Actor>(e);
    if (!actor_w) {
      auto transform_w = g_ent_mgr.GetNewCbeW<lib_graphics::Transform>(e);
      if (transform_w) transform_w->Move(dp);
    } else
      actor_w->Move(dp);
  }
}

void Unit::RotateUnit(lib_core::Vector3 dr) {
  for (auto& e : scoped_entities_) {
    auto actor_w = g_ent_mgr.GetNewCbeW<lib_physics::Actor>(e);
    if (!actor_w) {
      auto transform_w = g_ent_mgr.GetNewCbeW<lib_graphics::Transform>(e);
      if (transform_w) {
        transform_w->Yaw(dr[1]);
        transform_w->Pitch(dr[0]);
        transform_w->Roll(dr[2]);
      }
    } else
      actor_w->MovePose({0.f}, dr);
  }
}

void Unit::PositionUnit(lib_core::Vector3 position) {
  for (auto& e : scoped_entities_) {
    auto actor_w = g_ent_mgr.GetNewCbeW<lib_physics::Actor>(e);
    if (!actor_w) {
      auto transform_w = g_ent_mgr.GetNewCbeW<lib_graphics::Transform>(e);
      if (transform_w) transform_w->MoveTo(position);
    } else {
      auto transform = g_ent_mgr.GetNewCbeR<lib_graphics::Transform>(e);
      if (transform) {
        actor_w->SetPose(position, transform->rotation_ * 180 / PI);
      } else {
        lib_core::Vector3 rotation;
        actor_w->rot.GetAngles(rotation);
        actor_w->SetPose(position, rotation);
      }
    }
  }
}

void Unit::Activate() {
  active_ = true;

  for (auto& t : *stored_transforms_) g_ent_mgr.AddComponent(t.first, t.second);
  for (auto& m : *stored_meshes_) g_ent_mgr.AddComponent(m.first, m.second);
  for (auto& l : *stored_lights_) g_ent_mgr.AddComponent(l.first, l.second);
  for (auto& e : *stored_emitters_) g_ent_mgr.AddComponent(e.first, e.second);
  for (auto& a : *stored_actors_) g_ent_mgr.AddComponent(a.first, a.second);
  for (auto& j : *stored_joints_) g_ent_mgr.AddComponent(j.first, j.second);
  for (auto& t : *stored_triggers_) g_ent_mgr.AddComponent(t.first, t.second);
  stored_transforms_->clear();
  stored_meshes_->clear();
  stored_actors_->clear();
  stored_joints_->clear();
  stored_triggers_->clear();
  stored_lights_->clear();
  stored_emitters_->clear();
}

void Unit::Deactivate() {
  active_ = false;

  if (stored_transforms_->empty() && stored_meshes_->empty() &&
      stored_actors_->empty() && stored_joints_->empty() &&
      stored_triggers_->empty()) {
    for (auto& e : scoped_entities_) {
      auto transform = g_ent_mgr.GetNewCbeR<lib_graphics::Transform>(e);
      auto mesh = g_ent_mgr.GetNewCbeR<lib_graphics::Mesh>(e);
      auto light = g_ent_mgr.GetNewCbeR<lib_graphics::Light>(e);
      auto emitter = g_ent_mgr.GetNewCbeR<lib_graphics::ParticleEmitter>(e);
      auto actor = g_ent_mgr.GetNewCbeR<lib_physics::Actor>(e);
      auto joint = g_ent_mgr.GetNewCbeR<lib_physics::Joint>(e);
      auto trigger = g_ent_mgr.GetNewCbeR<lib_physics::Trigger>(e);

      if (mesh) {
        (*stored_meshes_)[e] = *mesh;
        g_ent_mgr.RemoveComponent<lib_graphics::Mesh>(e);
      }
      if (light) {
        (*stored_lights_)[e] = *light;
        g_ent_mgr.RemoveComponent<lib_graphics::Light>(e);
      }
      if (emitter) {
        (*stored_emitters_)[e] = *emitter;
        g_ent_mgr.RemoveComponent<lib_graphics::ParticleEmitter>(e);
      }
      if (actor) {
        (*stored_actors_)[e] = *actor;
        g_ent_mgr.RemoveComponent<lib_physics::Actor>(e);
      }
      if (joint) {
        (*stored_joints_)[e] = *joint;
        g_ent_mgr.RemoveComponent<lib_physics::Joint>(e);
      }
      if (trigger) {
        (*stored_triggers_)[e] = *trigger;
        g_ent_mgr.RemoveComponent<lib_physics::Trigger>(e);
      }
      if (transform) {
        (*stored_transforms_)[e] = *transform;
        g_ent_mgr.RemoveComponent<lib_graphics::Transform>(e);
      }
    }
  }
}

bool Unit::IsActive() { return active_; }

void Unit::PurgeEntities() {
  for (auto e : scoped_entities_) g_ent_mgr.RemoveEntity(e);
  scoped_entities_.clear();
}
}  // namespace lib_core
