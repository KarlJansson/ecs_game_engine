#include "physx_actor_handler.h"
#include "actor.h"
#include "entity_manager.h"
#include "joint.h"
#include "transform.h"

namespace lib_physics {
PhysxActorHandler::PhysxActorHandler(physx::PxPhysics* phys,
                                     physx::PxCooking* cook,
                                     physx::PxScene* scene)
    : physics_(phys), cooking_(cook), scene_(scene) {
  add_actor_callback_ = g_ent_mgr.RegisterAddComponentCallback<Actor>(
      [&](lib_core::Entity entity) { add_actor_.push_back(entity); });
  remove_actor_callback_ = g_ent_mgr.RegisterRemoveComponentCallback<Actor>(
      [&](lib_core::Entity entity) { remove_actor_.push_back(entity); });
}

PhysxActorHandler::~PhysxActorHandler() {
  g_ent_mgr.UnregisterAddComponentCallback<Actor>(add_actor_callback_);
  g_ent_mgr.UnregisterRemoveComponentCallback<Actor>(remove_actor_callback_);

  for (auto& p : actors_) p.second->release();
}

void PhysxActorHandler::Update() {
  auto actors = g_ent_mgr.GetNewCbt<Actor>();

  for (int i = int(add_actor_.size()) - 1; i >= 0; --i)
    if (AddNewActorData(add_actor_[i], scene_))
      add_actor_.erase(add_actor_.begin() + i);

  for (int i = int(remove_actor_.size()) - 1; i >= 0; --i)
    if (RemoveActorData(remove_actor_[i], scene_))
      remove_actor_.erase(remove_actor_.begin() + i);

  if (actors) {
    auto old_actors = g_ent_mgr.GetOldCbt<Actor>();
    auto actors_ents = g_ent_mgr.GetEbt<Actor>();
    auto actor_update = g_ent_mgr.GetNewUbt<Actor>();

    for (int i = 0; i < actor_update->size(); ++i) {
      if (!(*actor_update)[i]) continue;

      auto& a = actors->at(i);
      auto& old_a = old_actors->at(i);
      auto actor_it = actors_.find(actors_ents->at(i));
      if (actor_it == actors_.end()) continue;
      (*actor_update)[i] = false;

      a.pos = old_a.pos;
      a.rot = old_a.rot;

      auto body = static_cast<physx::PxRigidBody*>(actor_it->second);
      auto type = body->getType();
      if (type == physx::PxActorType::eRIGID_DYNAMIC) {
        auto rigid_dynamic =
            static_cast<physx::PxRigidDynamic*>(actor_it->second);
        if (a.type == Actor::kKinematic && old_a.type == Actor::kDynamic) {
          rigid_dynamic->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC,
                                          true);

          a.type = Actor::kKinematic;
        } else if (a.type == Actor::kDynamic &&
                   old_a.type == Actor::kKinematic) {
          rigid_dynamic->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC,
                                          false);
          a.type = Actor::kDynamic;
        }
      }

      if (a.type == Actor::kDynamic) {
        if (a.force[0] != 0.0f || a.force[1] != 0.0f || a.force[2] != 0.0f)
          body->addForce(physx::PxVec3(a.force[0], a.force[1], a.force[2]));
        if (a.torque[0] != 0.0f || a.torque[1] != 0.0f || a.torque[2] != 0.0f)
          body->addTorque(physx::PxVec3(a.torque[0], a.torque[1], a.torque[2]));
      }

      if (a.delta_pos[0] != 0.0f || a.delta_pos[1] != 0.0f ||
          a.delta_pos[2] != 0.0f) {
        auto pose = body->getGlobalPose();
        pose.p.x += a.delta_pos[0];
        pose.p.y += a.delta_pos[1];
        pose.p.z += a.delta_pos[2];

        if (a.type == Actor::kKinematic) {
          auto rigid_dynamic = static_cast<physx::PxRigidDynamic*>(body);
          rigid_dynamic->setKinematicTarget(pose);
        } else
          body->setGlobalPose(pose);

        if (a.type == Actor::kStatic) force_update_.insert(actors_ents->at(i));
      }

      if (a.set_velocity)
        body->setLinearVelocity({a.velocity[0], a.velocity[1], a.velocity[2]});

      if (a.set_pose || a.move_pose) {
        const float scaling = (PI / 180.0f) * 0.5f;
        auto t0 = std::cos(a.new_rot[2] * scaling);
        auto t1 = std::sin(a.new_rot[2] * scaling);
        auto t2 = std::cos(a.new_rot[0] * scaling);
        auto t3 = std::sin(a.new_rot[0] * scaling);
        auto t4 = std::cos(a.new_rot[1] * scaling);
        auto t5 = std::sin(a.new_rot[1] * scaling);

        physx::PxQuat q;
        q.w = t0 * t2 * t4 + t1 * t3 * t5;
        q.x = -(t0 * t3 * t4 - t1 * t2 * t5);
        q.y = -(t0 * t2 * t5 + t1 * t3 * t4);
        q.z = -(t1 * t2 * t4 - t0 * t3 * t5);

        switch (a.type) {
          case Actor::kStatic:
            force_update_.insert(actors_ents->at(i));
          case Actor::kDynamic:
            body->setGlobalPose(physx::PxTransform(a.new_pos[0], a.new_pos[1],
                                                   a.new_pos[2], q));
            break;
          case Actor::kKinematic: {
            auto rigid_dynamic = static_cast<physx::PxRigidDynamic*>(body);
            if (a.move_pose) {
              auto pose = rigid_dynamic->getGlobalPose();
              for (int ii = 0; ii < 3; ++ii) pose.p[ii] += a.new_pos[ii];
              rigid_dynamic->setKinematicTarget(pose);
            } else
              rigid_dynamic->setKinematicTarget(physx::PxTransform(
                  a.new_pos[0], a.new_pos[1], a.new_pos[2], q));
            break;
          }
          case Actor::kPlane:
            break;
        }
      }

      a.torque.ZeroMem();
      a.force.ZeroMem();
      a.delta_pos.ZeroMem();
      a.set_pose = false;
      a.move_pose = false;
      a.set_velocity = false;
    }
  }
}

void PhysxActorHandler::UpdateActors(
    ct::hash_set<physx::PxActor*>& active_actors) {
  for (auto physx_actor : active_actors) {
    if (!physx_actor->userData) continue;

    auto entity =
        lib_core::Entity(reinterpret_cast<size_t>(physx_actor->userData));
    auto actor = g_ent_mgr.GetNewCbeW<Actor>(entity);
    if (!actor) continue;
    force_update_.erase(entity);

    auto body = static_cast<physx::PxRigidBody*>(physx_actor);
    auto transform = body->getGlobalPose();
    actor->pos[0] = transform.p.x;
    actor->pos[1] = transform.p.y;
    actor->pos[2] = transform.p.z;
    actor->rot.x = -transform.q.x;
    actor->rot.y = -transform.q.y;
    actor->rot.z = -transform.q.z;
    actor->rot.w = transform.q.w;

    g_ent_mgr.MarkForUpdate<lib_graphics::Transform>(entity);
  }

  for (auto e : force_update_) {
    auto it = actors_.find(e);
    if (it == actors_.end()) continue;

    auto actor = g_ent_mgr.GetNewCbeW<Actor>(e);
    if (!actor) continue;

    auto body = static_cast<physx::PxRigidBody*>(it->second);
    auto transform = body->getGlobalPose();
    actor->pos[0] = transform.p.x;
    actor->pos[1] = transform.p.y;
    actor->pos[2] = transform.p.z;
    actor->rot.x = -transform.q.x;
    actor->rot.y = -transform.q.y;
    actor->rot.z = -transform.q.z;
    actor->rot.w = transform.q.w;

    g_ent_mgr.MarkForUpdate<lib_graphics::Transform>(e);
  }
  force_update_.clear();
}

physx::PxActor* PhysxActorHandler::CreatePlaneActor(const Actor& actor_comp,
                                                    float nx, float ny,
                                                    float nz, float dist) {
  auto material = physics_->createMaterial(actor_comp.static_friction,
                                           actor_comp.dynamic_friction,
                                           actor_comp.restitution);
  physx::PxRigidStatic* ground_plane = physx::PxCreatePlane(
      *physics_, physx::PxPlane(nx, ny, nz, dist), *material);
  material->release();
  return ground_plane;
}

bool PhysxActorHandler::AddNewActorData(lib_core::Entity entity,
                                        physx::PxScene* scene) {
  auto new_actor = g_ent_mgr.GetNewCbeR<Actor>(entity);
  if (!new_actor) return false;

  auto mesh_source = mesh_sources_.find(new_actor->mesh_id);
  if (mesh_source == mesh_sources_.end()) return false;

  auto actor_trans = g_ent_mgr.GetNewCbeR<lib_graphics::Transform>(entity);

  lib_core::Vector3 pos = new_actor->pos, rot = {0.0f, 0.0f, 0.0f},
                    scale = new_actor->scale;
  if (actor_trans) {
    pos = actor_trans->Position();
    rot = actor_trans->rotation_;
    scale = actor_trans->scale_ * new_actor->scale;
  }

  physx::PxActor* actor;
  switch (new_actor->type) {
    case Actor::kDynamic:
      if (new_actor->convex)
        actor = CreateDynamicConvexMeshActor(mesh_source->second.verts,
                                             *new_actor, pos, rot, scale,
                                             new_actor->ccd_collision);
      else
        actor = CreateDynamicTriangleMeshActor(
            mesh_source->second.verts, mesh_source->second.inds, *new_actor,
            pos, rot, scale, new_actor->ccd_collision);
      actor->userData = reinterpret_cast<void*>(size_t(entity.id_));
      scene->addActor(*actor);
      break;
    case Actor::kStatic:
      force_update_.insert(entity);
      actor = CreateStaticTriangleMeshActor(mesh_source->second.verts,
                                            mesh_source->second.inds,
                                            *new_actor, pos, rot, scale);
      actor->userData = reinterpret_cast<void*>(size_t(entity.id_));
      scene->addActor(*actor);
      break;
    case Actor::kKinematic: {
      if (new_actor->convex)
        actor = CreateDynamicConvexMeshActor(mesh_source->second.verts,
                                             *new_actor, pos, rot, scale,
                                             new_actor->ccd_collision);
      else
        actor = CreateDynamicTriangleMeshActor(
            mesh_source->second.verts, mesh_source->second.inds, *new_actor,
            pos, rot, scale, new_actor->ccd_collision);

      auto rigid_dynamic = static_cast<physx::PxRigidDynamic*>(actor);
      rigid_dynamic->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
      actor->userData = reinterpret_cast<void*>(size_t(entity.id_));
      scene->addActor(*actor);

      auto t0 = std::cos(rot[2] * 0.5f);
      auto t1 = std::sin(rot[2] * 0.5f);
      auto t2 = std::cos(rot[0] * 0.5f);
      auto t3 = std::sin(rot[0] * 0.5f);
      auto t4 = std::cos(rot[1] * 0.5f);
      auto t5 = std::sin(rot[1] * 0.5f);

      physx::PxQuat q;
      q.w = t0 * t2 * t4 + t1 * t3 * t5;
      q.x = -(t0 * t3 * t4 - t1 * t2 * t5);
      q.y = -(t0 * t2 * t5 + t1 * t3 * t4);
      q.z = -(t1 * t2 * t4 - t0 * t3 * t5);

      rigid_dynamic->setKinematicTarget(
          physx::PxTransform({pos[0], pos[1], pos[2]}, q));
      break;
    }
    case Actor::kPlane:
      actor = CreatePlaneActor(*new_actor, 0.0f, 1.0f, 0.0f, 0.0f);
      actor->userData = reinterpret_cast<void*>(size_t(entity.id_));
      scene->addActor(*actor);
      break;
    default:
      return false;
  }

  actors_[entity] = actor;
  return true;
}

bool PhysxActorHandler::RemoveActorData(lib_core::Entity entity,
                                        physx::PxScene* scene) {
  auto it = actors_.find(entity);
  if (it == actors_.end()) return false;

  auto actor = it->second;
  scene->removeActor(*actor, false);

  actor->release();
  actors_.erase(it);
  return true;
}

physx::PxActor* PhysxActorHandler::CreateStaticTriangleMeshActor(
    ct::dyn_array<lib_core::Vector3>& verts, ct::dyn_array<unsigned int>& inds,
    const Actor& actor_comp, lib_core::Vector3 pos, lib_core::Vector3 rot,
    lib_core::Vector3 scale) {
  physx::PxTolerancesScale tol_scale;
  physx::PxCookingParams params(tol_scale);
  params.meshPreprocessParams |=
      physx::PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
  // params.meshCookingHint = physx::PxMeshCookingHint::eCOOKING_PERFORMANCE;

  cooking_->setParams(params);

  physx::PxTriangleMeshDesc meshDesc;
  meshDesc.points.count = physx::PxU32(verts.size());
  meshDesc.points.stride = sizeof(physx::PxVec3);
  meshDesc.points.data = verts.data();

  meshDesc.triangles.count = physx::PxU32(inds.size() / 3);
  meshDesc.triangles.stride = 3 * sizeof(physx::PxU32);
  meshDesc.triangles.data = inds.data();

  physx::PxTriangleMesh* triangle_mesh = cooking_->createTriangleMesh(
      meshDesc, physics_->getPhysicsInsertionCallback());
  physx::PxTriangleMeshGeometry tri_geom;
  tri_geom.triangleMesh = triangle_mesh;
  tri_geom.scale.scale.x = scale[0], tri_geom.scale.scale.y = scale[1],
  tri_geom.scale.scale.z = scale[2];

  auto material = physics_->createMaterial(actor_comp.static_friction,
                                           actor_comp.dynamic_friction,
                                           actor_comp.restitution);
  physx::PxShape* shape = physics_->createShape(tri_geom, *material, false);

  float t0 = std::cos(rot[2] * 0.5f);
  float t1 = std::sin(rot[2] * 0.5f);
  float t2 = std::cos(rot[0] * 0.5f);
  float t3 = std::sin(rot[0] * 0.5f);
  float t4 = std::cos(rot[1] * 0.5f);
  float t5 = std::sin(rot[1] * 0.5f);

  physx::PxQuat q;
  q.w = t0 * t2 * t4 + t1 * t3 * t5;
  q.x = -(t0 * t3 * t4 - t1 * t2 * t5);
  q.y = -(t0 * t2 * t5 + t1 * t3 * t4);
  q.z = -(t1 * t2 * t4 - t0 * t3 * t5);
  physx::PxTransform transform(pos[0], pos[1], pos[2], q);

  physx::PxRigidStatic* actor =
      physx::PxCreateStatic(*physics_, transform, *shape);

  shape->release();
  triangle_mesh->release();
  material->release();
  return actor;
}

physx::PxActor* PhysxActorHandler::CreateDynamicConvexMeshActor(
    ct::dyn_array<lib_core::Vector3>& verts, const Actor& actor_comp,
    lib_core::Vector3 pos, lib_core::Vector3 rot, lib_core::Vector3 scale,
    bool ccd) {
  physx::PxTolerancesScale tol_scale;
  physx::PxCookingParams params(tol_scale);
  params.meshPreprocessParams |=
      physx::PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
  // params.meshCookingHint = physx::PxMeshCookingHint::eCOOKING_PERFORMANCE;

  cooking_->setParams(params);

  physx::PxConvexMeshDesc convexDesc;
  convexDesc.points.count = physx::PxU32(verts.size());
  convexDesc.points.stride = sizeof(physx::PxVec3);
  convexDesc.points.data = verts.data();
  convexDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

  physx::PxDefaultMemoryOutputStream buf;
  physx::PxConvexMeshCookingResult::Enum result;
  if (!cooking_->cookConvexMesh(convexDesc, buf, &result)) return nullptr;
  physx::PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
  physx::PxConvexMesh* convexMesh = physics_->createConvexMesh(input);

  float t0 = std::cos(rot[2] * 0.5f);
  float t1 = std::sin(rot[2] * 0.5f);
  float t2 = std::cos(rot[0] * 0.5f);
  float t3 = std::sin(rot[0] * 0.5f);
  float t4 = std::cos(rot[1] * 0.5f);
  float t5 = std::sin(rot[1] * 0.5f);

  physx::PxQuat q;
  q.w = t0 * t2 * t4 + t1 * t3 * t5;
  q.x = -(t0 * t3 * t4 - t1 * t2 * t5);
  q.y = -(t0 * t2 * t5 + t1 * t3 * t4);
  q.z = -(t1 * t2 * t4 - t0 * t3 * t5);

  auto mesh_geom = physx::PxConvexMeshGeometry(convexMesh);
  mesh_geom.scale.scale.x = scale[0];
  mesh_geom.scale.scale.y = scale[1];
  mesh_geom.scale.scale.z = scale[2];

  auto material = physics_->createMaterial(actor_comp.static_friction,
                                           actor_comp.dynamic_friction,
                                           actor_comp.restitution);
  physx::PxRigidDynamic* actor = physics_->createRigidDynamic(
      physx::PxTransform(pos[0], pos[1], pos[2], q));
  physx::PxRigidActorExt::createExclusiveShape(*actor, mesh_geom, *material);
  physx::PxRigidBodyExt::updateMassAndInertia(*actor, actor_comp.density);
  actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, ccd);

  convexMesh->release();
  material->release();
  return actor;
}

physx::PxActor* PhysxActorHandler::CreateDynamicTriangleMeshActor(
    ct::dyn_array<lib_core::Vector3>& verts, ct::dyn_array<unsigned int>& inds,
    const Actor& actor_comp, lib_core::Vector3 pos, lib_core::Vector3 rot,
    lib_core::Vector3 scale, bool ccd) {
  physx::PxTolerancesScale tol_scale;
  physx::PxCookingParams params(tol_scale);
  params.meshPreprocessParams |=
      physx::PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
  // params.meshCookingHint = physx::PxMeshCookingHint::eCOOKING_PERFORMANCE;

  cooking_->setParams(params);

  physx::PxTriangleMeshDesc meshDesc;
  meshDesc.points.count = physx::PxU32(verts.size());
  meshDesc.points.stride = sizeof(physx::PxVec3);
  meshDesc.points.data = verts.data();

  meshDesc.triangles.count = physx::PxU32(inds.size() / 3);
  meshDesc.triangles.stride = 3 * sizeof(physx::PxU32);
  meshDesc.triangles.data = inds.data();

  physx::PxTriangleMesh* triangle_mesh = cooking_->createTriangleMesh(
      meshDesc, physics_->getPhysicsInsertionCallback());
  physx::PxTriangleMeshGeometry tri_geom;
  tri_geom.triangleMesh = triangle_mesh;
  tri_geom.scale.scale.x = scale[0], tri_geom.scale.scale.y = scale[1],
  tri_geom.scale.scale.z = scale[2];

  auto material = physics_->createMaterial(actor_comp.static_friction,
                                           actor_comp.dynamic_friction,
                                           actor_comp.restitution);
  physx::PxShape* shape = physics_->createShape(tri_geom, *material, false);

  float t0 = std::cos(rot[2] * 0.5f);
  float t1 = std::sin(rot[2] * 0.5f);
  float t2 = std::cos(rot[0] * 0.5f);
  float t3 = std::sin(rot[0] * 0.5f);
  float t4 = std::cos(rot[1] * 0.5f);
  float t5 = std::sin(rot[1] * 0.5f);

  physx::PxQuat q;
  q.w = t0 * t2 * t4 + t1 * t3 * t5;
  q.x = -(t0 * t3 * t4 - t1 * t2 * t5);
  q.y = -(t0 * t2 * t5 + t1 * t3 * t4);
  q.z = -(t1 * t2 * t4 - t0 * t3 * t5);
  physx::PxTransform transform(pos[0], pos[1], pos[2], q);

  physx::PxRigidDynamic* actor = physics_->createRigidDynamic(transform);
  actor->attachShape(*shape);

  physx::PxRigidBodyExt::updateMassAndInertia(*actor, actor_comp.density);
  actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, ccd);

  shape->release();
  triangle_mesh->release();
  material->release();
  return actor;
}
}  // namespace lib_physics
