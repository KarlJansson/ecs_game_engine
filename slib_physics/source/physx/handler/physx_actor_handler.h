#pragma once
#include "PxPhysicsAPI.h"
#include "actor.h"
#include "actor_handler.h"
#include "vector_def.h"
#include "vertex.h"

namespace lib_physics {
class PhysxActorHandler : public ActorHandler {
 public:
  PhysxActorHandler(physx::PxPhysics* phys, physx::PxCooking* cook,
                    physx::PxScene* scene);
  ~PhysxActorHandler();

  void Update();
  void UpdateActors(ct::hash_set<physx::PxActor*>& active_actors);

  ct::hash_map<lib_core::Entity, physx::PxActor*> actors_;

 private:
  physx::PxActor* CreateStaticTriangleMeshActor(
      ct::dyn_array<lib_core::Vector3>& verts,
      ct::dyn_array<unsigned int>& inds, const Actor& actor_comp,
      lib_core::Vector3 pos = {0.0f, 0.0f, 0.0f},
      lib_core::Vector3 rot = {0.0f, 0.0f, 0.0f},
      lib_core::Vector3 scale = {1.0f, 1.0f, 1.0f});
  physx::PxActor* CreateDynamicConvexMeshActor(
      ct::dyn_array<lib_core::Vector3>& verts, const Actor& actor_comp,
      lib_core::Vector3 pos = {0.0f, 0.0f, 0.0f},
      lib_core::Vector3 rot = {0.0f, 0.0f, 0.0f},
      lib_core::Vector3 scale = {1.0f, 1.0f, 1.0f}, bool ccd = false);
  physx::PxActor* CreateDynamicTriangleMeshActor(
      ct::dyn_array<lib_core::Vector3>& verts,
      ct::dyn_array<unsigned int>& inds, const Actor& actor_comp,
      lib_core::Vector3 pos = {0.0f, 0.0f, 0.0f},
      lib_core::Vector3 rot = {0.0f, 0.0f, 0.0f},
      lib_core::Vector3 scale = {1.0f, 1.0f, 1.0f}, bool ccd = false);
  physx::PxActor* CreatePlaneActor(const Actor& actor_comp, float nx, float ny,
                                   float nz, float dist);

  bool AddNewActorData(lib_core::Entity entity, physx::PxScene* scene);
  bool RemoveActorData(lib_core::Entity entity, physx::PxScene* scene);

  physx::PxPhysics* physics_ = nullptr;
  physx::PxCooking* cooking_ = nullptr;
  physx::PxScene* scene_ = nullptr;

  ct::dyn_array<lib_core::Entity> add_actor_;
  ct::dyn_array<lib_core::Entity> remove_actor_;

  ct::hash_set<lib_core::Entity> force_update_;

  size_t add_actor_callback_, remove_actor_callback_;
};
}  // namespace lib_physics
