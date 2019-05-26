#include "transform.h"
#include "quaternion.h"

namespace lib_graphics {
Transform::Transform(lib_core::Vector3 pos, lib_core::Vector3 rot,
                     lib_core::Vector3 scale, lib_core::Vector3 orb,
                     lib_core::Vector3 orb_rot) {
  position_ = pos;
  orbit_offset_ = orb;
  orbit_rotation_ = orb_rot;

  auto rad_convert = (PI / 180.0f);
  rotation_ = rot;
  rotation_[0] *= rad_convert, rotation_[1] *= rad_convert,
      rotation_[2] *= rad_convert;
  orbit_rotation_[0] *= rad_convert, orbit_rotation_[1] *= rad_convert,
      orbit_rotation_[2] *= rad_convert;

  scale_ = scale;

  std::memset(world_.data, 0, sizeof(float) * 16);
  world_.data[0] = world_.data[5] = world_.data[11] = world_.data[15] = 1.0f;

  lib_core::Quaternion q, orb_q;
  float pi2 = PI * 2;
  for (int i = 0; i < 3; ++i) {
    if (rotation_[i] > pi2)
      rotation_[i] -= pi2;
    else if (rotation_[i] < -pi2)
      rotation_[i] += pi2;

    if (orbit_rotation_[i] > pi2)
      orbit_rotation_[i] -= pi2;
    else if (orbit_rotation_[i] < -pi2)
      orbit_rotation_[i] += pi2;
  }

  orb_q.FromAngle(orbit_rotation_);
  q.FromAngle(rotation_);

  auto orb_translate = orbit_offset_;
  if (!orb_translate.Zero()) orb_q.RotateVector(orb_translate);

  world_.Identity();
  world_.Translate(position_);
  if (!orb_translate.Zero()) world_.Translate(orb_translate);
  world_ *= q.RotationMatrix();
  world_.Scale(scale_);

  world_.Left(left_);
  world_.Up(up_);
  world_.Forward(forward_);

  left_.Normalize();
  up_.Normalize();
  forward_.Normalize();
}

lib_core::Vector3 Transform::Position() const {
  lib_core::Vector3 position;
  world_.Translation(position);
  return position;
}

void Transform::MoveForward(float amount) { position_ += forward_ * amount; }

void Transform::MoveBackward(float amount) { position_ -= forward_ * amount; }

void Transform::MoveUp(float amount) { position_ += up_ * amount; }

void Transform::MoveDown(float amount) { position_ -= up_ * amount; }

void Transform::MoveLeft(float amount) { position_ -= left_ * amount; }

void Transform::MoveRight(float amount) { position_ += left_ * amount; }

void Transform::Move(lib_core::Vector3 dir) { position_ += dir; }

void Transform::MoveTo(lib_core::Vector3 pos) { position_ = pos; }

void Transform::RotateTo(lib_core::Vector3 rot) { rotation_ = rot; }

void Transform::Yaw(float amount) { rotation_[1] -= amount; }

void Transform::Pitch(float amount) { rotation_[0] -= amount; }

void Transform::Roll(float amount) { rotation_[2] += amount; }

void Transform::ScaleX(float amount) { scale_[0] += amount; }

void Transform::ScaleY(float amount) { scale_[1] += amount; }

void Transform::ScaleZ(float amount) { scale_[2] += amount; }
}  // namespace lib_graphics
