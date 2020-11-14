#include "physx_system.h"
#include <thread>
#include "actor.h"
#include "character.h"
#include "entity_manager.h"
#include "gui_text.h"

namespace lib_physics {
PhysxSystem::PhysxSystem() {
  foundation_ = PxCreateFoundation(PX_FOUNDATION_VERSION, allocator_callback_,
                                   error_callback_);
  if (!foundation_) return;

  const bool record_memory_allocations = false;
  physics_ = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation_,
                             physx::PxTolerancesScale(),
                             record_memory_allocations, nullptr);
  if (!physics_) return;

  if (!PxInitExtensions(*physics_, nullptr)) return;

  cooking_ =
      PxCreateCooking(PX_PHYSICS_VERSION, *foundation_,
                      physx::PxCookingParams(physx::PxTolerancesScale()));
  if (!cooking_) return;

  physx::PxSceneDesc scene_desc(physics_->getTolerancesScale());
  tbb_dispatch_ = std::make_unique<TbbCpuDispatcher>();
  if (!tbb_dispatch_) return;
  scene_desc.cpuDispatcher = tbb_dispatch_.get();
  scene_desc.gravity.y = -9.82f;
  scene_desc.filterShader = &FilterShader;
  scene_desc.flags |= physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;
  scene_desc.flags |= physx::PxSceneFlag::eENABLE_CCD;

  if (!scene_desc.isValid()) return;
  scene_ = physics_->createScene(scene_desc);

  actor_handler_ =
      std::make_unique<PhysxActorHandler>(physics_, cooking_, scene_);
  joint_handler_ =
      std::make_unique<PhysxJointHandler>(physics_, actor_handler_.get());
  character_handler_ =
      std::make_unique<PhysxCharacterHandler>(physics_, scene_);
  trigger_handler_ = std::make_unique<PhysxTriggerHandler>(physics_, scene_);
}

PhysxSystem::~PhysxSystem() {
  joint_handler_.reset();
  character_handler_.reset();
  actor_handler_.reset();
  trigger_handler_.reset();

  if (scene_) scene_->release();
  if (physics_) physics_->release();
  if (cooking_) cooking_->release();
  PxCloseExtensions();
  if (foundation_) foundation_->release();
}

void PhysxSystem::LogicUpdate(float dt) {
  if (!scene_) return;
  ray_cast_results_[g_ent_mgr.GetNewIndex()].clear();

  BaseUpdate();

  character_handler_->UpdateCharacters(dt / time_multiplier_);
  actor_handler_->Update();
  joint_handler_->Update();
  trigger_handler_->Update();

  dt_ += dt / time_multiplier_;
  int max_iter = 0;
  const float max_wait = 1.f / 25.f;
  update_actors_.clear();
  physx::PxU32 nb_active_actors;
  physx::PxActor **active_actors;
  physx::PxU32 error = 0;
  std::chrono::duration<float> dur;
  while (dt_ > step_frequency && max_iter < 20) {
    scene_->simulate(step_frequency * time_multiplier_);
    auto start = std::chrono::high_resolution_clock::now();

    while (!scene_->fetchResults(false, &error)) {
      std::this_thread::sleep_for(0ms);

      // HACK: avoid infinite looping when physx shits the bed.
      dur = std::chrono::high_resolution_clock::now() - start;
      if (dur.count() > max_wait) {
        cu::Log("PhysX failed to fetch results in time.", __FILE__, __LINE__);
        break;
      }
    }

    active_actors = scene_->getActiveActors(nb_active_actors);
    for (physx::PxU32 i = 0; i < nb_active_actors; ++i)
      update_actors_.insert(active_actors[i]);

    dt_ -= step_frequency;
    ++max_iter;
  }
  actor_handler_->UpdateActors(update_actors_);

  auto &ori = Ts<physx::PxVec3>();
  auto &unit_dir = Ts<physx::PxVec3>();
  while (ray_cast_queue_.try_pop(ray_cast_tmp_)) {
    auto &cast_desc = ray_cast_tmp_.second;
    ori.x = cast_desc.origin[0];
    ori.y = cast_desc.origin[1];
    ori.z = cast_desc.origin[2];

    unit_dir.x = cast_desc.dir[0];
    unit_dir.y = cast_desc.dir[1];
    unit_dir.z = cast_desc.dir[2];
    unit_dir.normalize();

    physx::PxRaycastBuffer hit;
    physx::PxQueryFlags flags;
    if (cast_desc.dyn_hits && cast_desc.stat_hits)
      flags = physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::eSTATIC;
    else if (cast_desc.dyn_hits)
      flags = physx::PxQueryFlag::eDYNAMIC;
    else if (cast_desc.stat_hits)
      flags = physx::PxQueryFlag::eSTATIC;
    auto filter_data = physx::PxQueryFilterData(flags);

    bool status;
    int actor_id = -1;
    float dist = -1.0;
    status = scene_->raycast(ori, unit_dir, cast_desc.max_dist, hit,
                             physx::PxHitFlag::eDEFAULT, filter_data);

    if (status && hit.block.actor->userData) {
      actor_id = int(reinterpret_cast<size_t>(hit.block.actor->userData));
      dist = std::abs((hit.block.actor->getGlobalPose().p - ori).magnitude());
    } else if (status)
      dist = hit.block.distance;

    ray_cast_results_[g_ent_mgr.GetNewIndex()][ray_cast_tmp_.first] = {actor_id,
                                                                       dist};
  }
}

uint32_t PhysxSystem::RayCast(RayCastDesc cast_desc) {
  auto id = ray_cast_id_++;
  ray_cast_queue_.push({id, cast_desc});
  return id;
}

bool PhysxSystem::GetRayCastResult(uint32_t id, std::pair<int, float> &out) {
  auto it = ray_cast_results_[g_ent_mgr.GetNewIndex()].find(id);
  if (it != ray_cast_results_[g_ent_mgr.GetNewIndex()].end()) {
    out = it->second;
    return true;
  }
  return false;
}

CharacterHandler &PhysxSystem::GetCharacterHandler() {
  return *character_handler_;
}

ActorHandler &PhysxSystem::GetActorHandler() { return *actor_handler_; }

physx::PxFilterFlags PhysxSystem::FilterShader(
    physx::PxFilterObjectAttributes attributes0,
    physx::PxFilterData filterData0,
    physx::PxFilterObjectAttributes attributes1,
    physx::PxFilterData filterData1, physx::PxPairFlags &pairFlags,
    const void *constantBlock, physx::PxU32 constantBlockSize) {
  if (physx::PxFilterObjectIsTrigger(attributes0) ||
      physx::PxFilterObjectIsTrigger(attributes1)) {
    pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
    return physx::PxFilterFlag::eDEFAULT;
  }

  auto result = physx::PxDefaultSimulationFilterShader(
      attributes0, filterData0, attributes1, filterData1, pairFlags,
      constantBlock, constantBlockSize);
  // pairFlags |= physx::PxPairFlag::eSOLVE_CONTACT;
  // pairFlags |= physx::PxPairFlag::eDETECT_DISCRETE_CONTACT;
  pairFlags |= physx::PxPairFlag::eDETECT_CCD_CONTACT;
  return result;
}

PhysxSystem::TbbCpuDispatcher::TbbCpuDispatcher() noexcept {
  nr_threads_ = tbb::task_scheduler_init::default_num_threads();
}

void PhysxSystem::TbbCpuDispatcher::submitTask(physx::PxBaseTask &task) {
  task_group_.run([&task]() {
    task.run();
    task.release();
  });
}

uint32_t PhysxSystem::TbbCpuDispatcher::getWorkerCount() const {
  return nr_threads_;
}

void PhysxSystem::PhysxErrorCallback::reportError(physx::PxErrorCode::Enum code,
                                                  const char *message,
                                                  const char *file, int line) {
  /*std::ofstream out("./physx_error.log", std::ios::app);
  out << code << std::endl;
  out << message << std::endl;
  out << file << std::endl;
  out << line << std::endl;
  out.close();*/
  cu::Log(message, __FILE__, __LINE__);
}

void *PhysxSystem::PhysxAllocatorCallback::allocate(size_t size,
                                                    const char *typeName,
                                                    const char *filename,
                                                    int line) {
#ifdef WindowsBuild
  auto ptr = _aligned_malloc(size, 16);
#elif UnixBuild
  auto ptr = aligned_alloc(16, size);
#endif
  // total_allocation_ += size;
  // allocation_map_[ptr] = size;
  return ptr;
}

void PhysxSystem::PhysxAllocatorCallback::deallocate(void *ptr) {
// total_allocation_ -= allocation_map_[ptr];
// allocation_map_.erase(ptr);
#ifdef WindowsBuild
  _aligned_free(ptr);
#elif UnixBuild
  free(ptr);
#endif
}
}  // namespace lib_physics
