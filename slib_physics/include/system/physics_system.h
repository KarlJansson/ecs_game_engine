#pragma once
#include "system.h"
#include "vector_def.h"
#include "vertex.h"

namespace lib_physics {
class PhysicsSystem : public lib_core::System {
 public:
  PhysicsSystem() = default;
  virtual ~PhysicsSystem() = default;

  struct RayCastDesc {
    lib_core::Vector3 origin = {0.f};
    lib_core::Vector3 dir = {1.f, 0.f, 0.f};
    float max_dist = 100.f;
    bool dyn_hits = false;
    bool stat_hits = false;
  };

  virtual uint32_t RayCast(RayCastDesc cast_desc) = 0;
  virtual bool GetRayCastResult(uint32_t id, std::pair<int, float>& out) = 0;

  virtual class CharacterHandler& GetCharacterHandler() = 0;
  virtual class ActorHandler& GetActorHandler() = 0;

  void SetTimeMultiplier(float multiplier);
  float TimeMultiplier();

 protected:
  void BaseUpdate();

  std::atomic<float> time_multiplier_ = {1.f};
};
}  // namespace lib_physics
