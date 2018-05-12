#pragma once
#include "camera.h"
#include "camera_system.h"
#include "character.h"

namespace lib_graphics {
class GlCameraSystem : public CameraSystem {
 public:
  GlCameraSystem() = default;
  ~GlCameraSystem() = default;

  void LogicUpdate(float dt) override;

  static void ExtractPlane(Camera::Plane &plane, float *mat, int row);

 protected:
 private:
  void UpdateCamera(Camera &cam, Camera &old,
                    const lib_physics::Character *actor);
};
}  // namespace lib_graphics
