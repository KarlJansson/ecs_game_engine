#pragma once
#include "engine_core.h"
#include "system.h"

namespace lib_graphics {
struct TextureDesc;
class Camera;
class ParticleSystem : public lib_core::System {
 public:
  ParticleSystem(const lib_core::EngineCore* engine);

  void LogicUpdate(float dt) override;

  virtual void DrawParticleEmitter(lib_core::Entity entity,
                                   const lib_graphics::Camera& camera,
                                   const TextureDesc& depth_desc) = 0;
  virtual void PurgeGpuResources() = 0;
  virtual void RebuildGpuResources() = 0;

 protected:
  const lib_core::EngineCore* engine_;
};
}  // namespace lib_graphics