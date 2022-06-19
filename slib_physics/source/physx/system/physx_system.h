#pragma once
#include "entity.h"
#include "physics_commands.h"
#include "physics_system.h"
#include "physx_actor_handler.h"
#include "physx_character_handler.h"
#include "physx_joint_handler.h"
#include "physx_trigger_handler.h"
#include "vector_def.h"

#include <future>

namespace lib_physics {
class PhysxSystem : public PhysicsSystem {
 public:
  PhysxSystem();
  ~PhysxSystem() override;

  void LogicUpdate(float dt) override;

  uint32_t RayCast(RayCastDesc cast_desc) override;
  bool GetRayCastResult(uint32_t id, std::pair<int, float>& out) override;

  CharacterHandler& GetCharacterHandler() override;
  ActorHandler& GetActorHandler() override;

 protected:
 private:
  class TbbCpuDispatcher : public physx::PxCpuDispatcher {
   public:
    TbbCpuDispatcher() noexcept;
    ~TbbCpuDispatcher() noexcept override {}  // NOLINT

    void submitTask(physx::PxBaseTask& task) override;
    [[nodiscard]] uint32_t getWorkerCount() const override;

    uint32_t nr_threads_;
    ct::dyn_array<std::future<void>> task_group_;
  };

  class PhysxErrorCallback : public physx::PxErrorCallback {
   public:
    void reportError(physx::PxErrorCode::Enum code, const char* message,
                     const char* file, int line) override;
  };

  class PhysxAllocatorCallback : public physx::PxAllocatorCallback {
   public:
    void* allocate(size_t size, const char* typeName, const char* filename,
                   int line) override;
    void deallocate(void* ptr) override;

    size_t total_allocation_ = 0;
    ct::hash_map<void*, size_t> allocation_map_;
  };

  static physx::PxFilterFlags FilterShader(
      physx::PxFilterObjectAttributes attributes0,
      physx::PxFilterData filterData0,
      physx::PxFilterObjectAttributes attributes1,
      physx::PxFilterData filterData1, physx::PxPairFlags& pairFlags,
      const void* constantBlock, physx::PxU32 constantBlockSize);

  float dt_ = 0.f;
  const float step_frequency = 1.0f / 60.0f;
  PhysxErrorCallback error_callback_;
  PhysxAllocatorCallback allocator_callback_;

  physx::PxFoundation* foundation_ = nullptr;
  physx::PxPhysics* physics_ = nullptr;
  physx::PxScene* scene_ = nullptr;
  physx::PxCooking* cooking_ = nullptr;

  std::unique_ptr<TbbCpuDispatcher> tbb_dispatch_;

  std::atomic<uint32_t> ray_cast_id_ = {0};
  std::array<ct::tree_map<uint32_t, std::pair<int, float>>, 2>
      ray_cast_results_;
  tbb::concurrent_queue<std::pair<int, RayCastDesc>> ray_cast_queue_;

  std::unique_ptr<PhysxJointHandler> joint_handler_;
  std::unique_ptr<PhysxCharacterHandler> character_handler_;
  std::unique_ptr<PhysxActorHandler> actor_handler_;
  std::unique_ptr<PhysxTriggerHandler> trigger_handler_;

  ct::hash_set<physx::PxActor*> update_actors_;

  std::pair<int, RayCastDesc> ray_cast_tmp_;
};
}  // namespace lib_physics
