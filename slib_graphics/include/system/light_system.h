#pragma once
#include "component/camera.h"
#include "system.h"

namespace lib_graphics {
class LightSystem : public lib_core::System {
 public:
  LightSystem() = default;
  ~LightSystem() override = default;

  void LogicUpdate(float dt) override;

  static ct::dyn_array<lib_core::Matrix4x4> GetShadowMatrices(
      Light& light, bool culling = true);

 private:
  static void CalculateDirLightCascades(
      Light& light, ct::dyn_array<lib_core::Matrix4x4>& shadow_cascades,
      bool culling);
  static float ApplyCropMatrix(
      Camera::FrustumInfo& f, Camera& cam,
      ct::dyn_array<lib_core::Matrix4x4>& shadow_cascades);
};
}  // namespace lib_graphics
