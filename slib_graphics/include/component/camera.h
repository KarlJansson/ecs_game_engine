#pragma once
#include "core_utilities.h"
#include "vector_def.h"

namespace lib_graphics {
class Camera {
 public:
  Camera(lib_core::Vector3 pos, lib_core::Vector3 rot,
         float a_ratio = 16.0f / 9.0f, float fov = 55.0f, float n_plane = 0.1f,
         float f_plane = 650.0f);
  ~Camera() = default;

  struct Plane {
    lib_core::Vector3 normal;
    float w;
  };

  struct FrustumInfo {
    float neard, fard;
    float fov, ratio;
    lib_core::Vector3 point[8];
  };

  struct FrustumPlanes {
    Plane planes[6];
  };

  static ct::dyn_array<FrustumPlanes> CalculateFrustumGrid(FrustumInfo& frustum,
                                                           int x, int y);
  static FrustumInfo CalculateFrustumInfo(Camera& cam);
  static FrustumPlanes CalculateFrustumPlanes(FrustumInfo& b);

  void Move(lib_core::Vector3 dist);

  void Yaw(float amount);
  void Pitch(float amount);
  void Roll(float amount);

  void SetFov(float fov);
  void SetAspectRatio(float a_ratio);
  void SetNearPlane(float near_plane);
  void SetFarPlane(float far_plane);
  void SetRotation(lib_core::Vector3 rot);
  void SetExposure(float exposure);

  lib_core::Vector3 Forward(const Camera& old) const;
  lib_core::Vector3 Up(const Camera& old) const;
  lib_core::Vector3 Left(const Camera& old) const;
  lib_core::Vector3 Rotation(const Camera& old) const;
  lib_core::Vector3 Position(const Camera& old) const;

  lib_core::Vector3 position_, rotation_;
  lib_core::Vector3 forward_, up_, left_;
  lib_core::Vector3 delta_pos_, delta_rot_;
  float fov_, a_ratio_, near_, far_;
  bool set_flags_[6];

  float exposure_;
  lib_core::Matrix4x4 view_, proj_, view_proj_;
  FrustumPlanes planes_;
};
}  // namespace lib_graphics
