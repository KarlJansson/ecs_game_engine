#include "camera.h"

#include <cmath>
#include "quaternion.h"

namespace lib_graphics {
Camera::Camera(lib_core::Vector3 pos, lib_core::Vector3 rot, float a_ratio,
               float fov, float n_plane, float f_plane, float orbit)
    : fov_(fov),
      a_ratio_(a_ratio),
      near_(n_plane),
      far_(f_plane),
      orbit_(orbit) {
  position_ = pos;

  memset(view_.data, 0, sizeof(float) * 16);
  memset(view_proj_.data, 0, sizeof(float) * 16);
  memset(proj_.data, 0, sizeof(float) * 16);

  exposure_ = 1.0f;

  for (bool& set_flag : set_flags_) set_flag = false;

  constexpr auto rad_convert = (PI / 180.0f);
  rotation_ = rot;
  rotation_[0] *= rad_convert;
  rotation_[1] *= rad_convert;
  rotation_[2] *= rad_convert;

  delta_pos_.ZeroMem();
  delta_rot_.ZeroMem();
}

ct::dyn_array<Camera::FrustumPlanes> Camera::CalculateFrustumGrid(
    Camera::FrustumInfo& frustum, int x, int y) {
  auto x_step_f = (frustum.point[1] - frustum.point[0]) / float(x);
  auto y_step_f = (frustum.point[2] - frustum.point[0]) / float(y);
  auto x_step_n = (frustum.point[5] - frustum.point[4]) / float(x);
  auto y_step_n = (frustum.point[6] - frustum.point[4]) / float(y);

  Camera::FrustumInfo tmp_frustum;
  ct::dyn_array<Camera::FrustumPlanes> grid;
  for (size_t i = 0; i < x; ++i) {
    for (size_t ii = 0; ii < y; ++ii) {
      // Far plane
      tmp_frustum.point[0] =
          frustum.point[0] + x_step_f * float(i) + y_step_f * float(ii);
      tmp_frustum.point[1] =
          frustum.point[0] + x_step_f * float(i + 1) + y_step_f * float(ii);
      tmp_frustum.point[2] = tmp_frustum.point[0] + y_step_f;
      tmp_frustum.point[3] = tmp_frustum.point[1] + y_step_f;

      // Near plane
      tmp_frustum.point[4] =
          frustum.point[4] + x_step_n * float(i) + y_step_n * float(ii);
      tmp_frustum.point[5] =
          frustum.point[4] + x_step_n * float(i + 1) + y_step_n * float(ii);
      tmp_frustum.point[6] = tmp_frustum.point[4] + y_step_n;
      tmp_frustum.point[7] = tmp_frustum.point[5] + y_step_n;

      grid.push_back(CalculateFrustumPlanes(tmp_frustum));
    }
  }

  return grid;
}

Camera::FrustumInfo Camera::CalculateFrustumInfo(Camera& cam) {
  FrustumInfo b;
  b.neard = cam.near_;
  b.fard = cam.far_;
  b.ratio = cam.a_ratio_;
  b.fov = cam.fov_ * (PI / 180.f);

  auto near_center = cam.position_ - cam.forward_ * cam.near_;
  auto far_center = cam.position_ - cam.forward_ * cam.far_;

  auto near_height = 2.f * std::tan(b.fov * .5f) * cam.near_;
  auto far_height = 2.f * std::tan(b.fov * .5f) * cam.far_;
  auto near_width = near_height * cam.a_ratio_;
  auto far_width = far_height * cam.a_ratio_;

  auto cam_left = cam.left_ * (far_width * .5f);
  auto cam_up = cam.up_ * (far_height * .5f);
  auto top = far_center + cam_up;
  auto bottom = far_center - cam_up;

  // Far plane
  b.point[0] = top + cam_left;     // Top left
  b.point[1] = top - cam_left;     // Top right
  b.point[2] = bottom + cam_left;  // Bottom left
  b.point[3] = bottom - cam_left;  // Bottom right

  cam_left = cam.left_ * (near_width * .5f);
  cam_up = cam.up_ * (near_height * .5f);
  top = near_center + cam_up;
  bottom = near_center - cam_up;

  // Near plane
  b.point[4] = top + cam_left;     // Top left
  b.point[5] = top - cam_left;     // Top right
  b.point[6] = bottom + cam_left;  // Bottom left
  b.point[7] = bottom - cam_left;  // Bottom right

  return b;
}

Camera::FrustumPlanes Camera::CalculateFrustumPlanes(Camera::FrustumInfo& b) {
  FrustumPlanes p;

  // Left plane
  auto p0 = b.point[6], p1 = b.point[2], p2 = b.point[0];
  p.planes[0].normal = p1 - p0;
  p.planes[0].normal.Cross(p2 - p1);
  p.planes[0].normal.Normalize();
  p.planes[0].w = p.planes[0].normal.Dot(p0);

  // Top plane
  p0 = b.point[4], p1 = b.point[0], p2 = b.point[1];
  p.planes[1].normal = p1 - p0;
  p.planes[1].normal.Cross(p2 - p1);
  p.planes[1].normal.Normalize();
  p.planes[1].w = p.planes[1].normal.Dot(p0);

  // Right plane
  p0 = b.point[5], p1 = b.point[1], p2 = b.point[3];
  p.planes[2].normal = p1 - p0;
  p.planes[2].normal.Cross(p2 - p1);
  p.planes[2].normal.Normalize();
  p.planes[2].w = p.planes[2].normal.Dot(p0);

  // Bottom plane
  p0 = b.point[7], p1 = b.point[3], p2 = b.point[2];
  p.planes[3].normal = p1 - p0;
  p.planes[3].normal.Cross(p2 - p1);
  p.planes[3].normal.Normalize();
  p.planes[3].w = p.planes[3].normal.Dot(p0);

  // Far plane
  p0 = b.point[4], p1 = b.point[5], p2 = b.point[6];
  p.planes[4].normal = p1 - p0;
  p.planes[4].normal.Cross(p2 - p1);
  p.planes[4].normal.Normalize();
  p.planes[4].w = p.planes[4].normal.Dot(p0);

  // Near plane
  p0 = b.point[0], p1 = b.point[1], p2 = b.point[2];
  p.planes[5].normal = p1 - p0;
  p.planes[5].normal.Cross(p2 - p1);
  p.planes[5].normal.Normalize();
  p.planes[5].w = p.planes[5].normal.Dot(p0);

  return p;
}

void Camera::Move(lib_core::Vector3 dist) { delta_pos_ += dist; }

void Camera::Yaw(float amount) { delta_rot_[1] -= amount; }

void Camera::Pitch(float amount) { delta_rot_[0] -= amount; }

void Camera::Roll(float amount) { delta_rot_[2] += amount; }

void Camera::SetFov(float fov) {
  fov_ = fov;
  set_flags_[2] = true;
}

void Camera::SetAspectRatio(float a_ratio) {
  a_ratio_ = a_ratio;
  set_flags_[3] = true;
}

void Camera::SetNearPlane(float near_plane) {
  near_ = near_plane;
  set_flags_[4] = true;
}

void Camera::SetFarPlane(float far_plane) {
  far_ = far_plane;
  set_flags_[5] = true;
}

void Camera::SetRotation(lib_core::Vector3 rot) {
  constexpr auto rad_convert = (PI / 180.0f);
  delta_rot_ = rot;
  delta_rot_[0] *= rad_convert;
  delta_rot_[1] *= rad_convert;
  delta_rot_[2] *= rad_convert;
  set_flags_[0] = true;
}

void Camera::SetExposure(float exposure) {
  exposure_ = exposure;
  set_flags_[1] = true;
}

lib_core::Vector3 Camera::Forward(const Camera& old) const {
  auto result = lib_core::Vector3(0.f, 0.f, 1.f);
  lib_core::Quaternion q;
  q.FromAngle(Rotation(old));
  q.RotateVector(result);
  return result;
}

lib_core::Vector3 Camera::Up(const Camera& old) const {
  auto result = lib_core::Vector3(0.f, 1.f, 0.f);
  lib_core::Quaternion q;
  q.FromAngle(Rotation(old));
  q.RotateVector(result);
  return result;
}

lib_core::Vector3 Camera::Left(const Camera& old) const {
  auto result = lib_core::Vector3(-1.f, 0.f, 0.f);
  lib_core::Quaternion q;
  q.FromAngle(Rotation(old));
  q.RotateVector(result);
  return result;
}

lib_core::Vector3 Camera::Rotation(const Camera& old) const {
  auto result = old.rotation_ + delta_rot_;

  float pi2 = PI * 2;
  for (int i = 0; i < 3; ++i) {
    if (result[i] > pi2)
      result[i] -= pi2;
    else if (result[i] < -pi2)
      result[i] += pi2;
  }

  if (result[0] > PI * 0.49f) result[0] = PI * 0.49f;
  if (result[0] < -PI * 0.49f) result[0] = -PI * 0.49f;

  return result;
}

lib_core::Vector3 Camera::Position(const Camera& old) const {
  return old.position_ + delta_pos_;
}
}  // namespace lib_graphics
