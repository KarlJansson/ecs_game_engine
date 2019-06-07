#include "physx_trigger_handler.h"
#include "actor.h"
#include "entity_manager.h"
#include "transform.h"
#include "trigger.h"

namespace lib_physics {
PhysxTriggerHandler::PhysxTriggerHandler(physx::PxPhysics *physics,
                                         physx::PxScene *scene)
    : physics_(physics), scene_(scene) {
  trigger_callback = std::make_unique<PhysxTriggerCallback>();
  default_material_ = physics->createMaterial(0.5f, 0.5f, 0.5f);
  scene->setSimulationEventCallback(trigger_callback.get());

  add_trigger_callback_ = g_ent_mgr.RegisterAddComponentCallback<Trigger>(
      [&](lib_core::Entity entity) { add_triggers_.push_back(entity); });
  remove_trigger_callback_ = g_ent_mgr.RegisterRemoveComponentCallback<Trigger>(
      [&](lib_core::Entity entity) { remove_triggers_.push_back(entity); });
}

PhysxTriggerHandler::~PhysxTriggerHandler() {
  g_ent_mgr.UnregisterAddComponentCallback<Trigger>(add_trigger_callback_);
  g_ent_mgr.UnregisterRemoveComponentCallback<Trigger>(
      remove_trigger_callback_);
  default_material_->release();
}

void PhysxTriggerHandler::Update() {
  for (auto e : add_triggers_) AddTrigger(e);
  add_triggers_.clear();

  for (auto e : remove_triggers_) RemoveTrigger(e);
  remove_triggers_.clear();

  auto trigger_update = g_ent_mgr.GetNewUbt<lib_physics::Trigger>();
  if (trigger_update) {
    auto trigger_ents = g_ent_mgr.GetEbt<lib_physics::Trigger>();
    auto triggers = g_ent_mgr.GetNewCbt<lib_physics::Trigger>();
    auto old_triggers = g_ent_mgr.GetOldCbt<lib_physics::Trigger>();

    for (size_t i = 0; i < trigger_update->size(); ++i) {
      if ((*trigger_update)[i]) {
        auto transform =
            g_ent_mgr.GetNewCbeR<lib_graphics::Transform>((*trigger_ents)[i]);
        if (transform) {
          auto trigger_actor = triggers_.find((*trigger_ents)[i]);
          if (trigger_actor != triggers_.end()) {
            lib_core::Vector3 rotation = transform->rotation_ * .5f;
            auto t0 = std::cos(rotation[2]);
            auto t1 = std::sin(rotation[2]);
            auto t2 = std::cos(rotation[0]);
            auto t3 = std::sin(rotation[0]);
            auto t4 = std::cos(rotation[1]);
            auto t5 = std::sin(rotation[1]);

            physx::PxQuat q;
            q.w = t0 * t2 * t4 + t1 * t3 * t5;
            q.x = -(t0 * t3 * t4 - t1 * t2 * t5);
            q.y = -(t0 * t2 * t5 + t1 * t3 * t4);
            q.z = -(t1 * t2 * t4 - t0 * t3 * t5);

            auto t_pos = transform->Position();
            trigger_actor->second->setGlobalPose(
                physx::PxTransform({t_pos[0], t_pos[1], t_pos[2]}, q));

            (*triggers)[i].pos = t_pos;
            (*triggers)[i].rot = transform->rotation_ * (180 / PI);
            (*old_triggers)[i].pos = (*triggers)[i].pos;
            (*old_triggers)[i].rot = (*triggers)[i].rot;
          }
        }

        (*trigger_update)[i] = false;
      }
    }
  }
}

void PhysxTriggerHandler::AddTrigger(lib_core::Entity e) {
  auto trigger = g_ent_mgr.GetNewCbeR<Trigger>(e);
  if (trigger) {
    lib_core::Vector3 position, rotation, scale;
    auto transform = g_ent_mgr.GetNewCbeR<lib_graphics::Transform>(e);
    if (transform) {
      position = transform->Position();
      rotation = transform->rotation_;
      scale = transform->scale_;
    } else {
      position = trigger->pos;
      rotation = trigger->rot * (PI / 180.0f);
      scale = trigger->size;
    }

    physx::PxShape *shape;
    physx::PxRigidActor *actor;
    switch (trigger->type) {
      case Trigger::kSphere:
        shape = physics_->createShape(physx::PxSphereGeometry(scale[0]),
                                      *default_material_, true);
        break;
      case Trigger::kBox:
        shape = physics_->createShape(
            physx::PxBoxGeometry(physx::PxVec3({scale[0], scale[1], scale[2]})),
            *default_material_, true);
        break;
      default:
        cu::AssertWarning(false, "Faulty trigger type specified.", __FILE__,
                          __LINE__);
        return;
    }

    rotation *= .5f;
    auto t0 = std::cos(rotation[2]);
    auto t1 = std::sin(rotation[2]);
    auto t2 = std::cos(rotation[0]);
    auto t3 = std::sin(rotation[0]);
    auto t4 = std::cos(rotation[1]);
    auto t5 = std::sin(rotation[1]);

    physx::PxQuat q;
    q.w = t0 * t2 * t4 + t1 * t3 * t5;
    q.x = -(t0 * t3 * t4 - t1 * t2 * t5);
    q.y = -(t0 * t2 * t5 + t1 * t3 * t4);
    q.z = -(t1 * t2 * t4 - t0 * t3 * t5);

    auto px_transform =
        physx::PxTransform({position[0], position[1], position[2]}, q);

    actor = physics_->createRigidStatic(px_transform);

    shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
    shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
    shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);

    actor->attachShape(*shape);
    if (trigger->trigger_callback)
      trigger_callback->trigger_map[e] = trigger->trigger_callback;
    actor->userData = reinterpret_cast<void *>(e.id_);
    shape->release();

    scene_->addActor(*actor);

    triggers_[e] = actor;
  }
}

void PhysxTriggerHandler::RemoveTrigger(lib_core::Entity e) {
  auto it = triggers_.find(e);
  if (it != triggers_.end()) {
    trigger_callback->trigger_map.erase(e);
    scene_->removeActor(*it->second);
    it->second->release();
    triggers_.erase(it);
  }
}

void PhysxTriggerHandler::PhysxTriggerCallback::onTrigger(
    physx::PxTriggerPair *pairs, physx::PxU32 count) {
  lib_core::Entity ents[2];
  for (physx::PxU32 i = 0; i < count; ++i) {
    if (pairs[i].triggerActor && pairs[i].triggerActor->userData &&
        pairs[i].otherActor && pairs[i].otherActor->userData) {
      ents[0] = lib_core::Entity(
          reinterpret_cast<size_t>(pairs[i].triggerActor->userData));
      ents[1] = lib_core::Entity(
          reinterpret_cast<size_t>(pairs[i].otherActor->userData));

      auto it = trigger_map.find(ents[0]);
      if (it != trigger_map.end()) it->second(ents[1]);
    }
  }
}
}  // namespace lib_physics
