#include "quaternion.h"
#include <cmath>
#include "core_utilities.h"

namespace lib_core {
const Quaternion Quaternion::operator-() const {
  return Quaternion(-x, -y, -z, -w);
}

Quaternion Quaternion::operator*(const Quaternion& rhs) const {
  Quaternion q;
  q.x = x * rhs.w + y * rhs.z - z * rhs.y + w * rhs.x;
  q.y = -x * rhs.z + y * rhs.w + z * rhs.x + w * rhs.y;
  q.z = x * rhs.y - y * rhs.x + z * rhs.w + w * rhs.z;
  q.w = -x * rhs.x - y * rhs.y - z * rhs.z + w * rhs.w;
  return q;
}

void Quaternion::operator*=(const Quaternion& rhs) { *this = *this * rhs; }

Quaternion Quaternion::operator*(const float rhs) const {
  return Quaternion(x * rhs, y * rhs, z * rhs, w * rhs);
}

void Quaternion::operator*=(const float rhs) {
  x *= rhs, y *= rhs, z *= rhs, w *= rhs;
}

Quaternion Quaternion::operator+(const Quaternion& rhs) const {
  return Quaternion(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
}

void Quaternion::operator+=(const Quaternion& rhs) {
  x += rhs.x, y += rhs.y, z += rhs.z, w += rhs.w;
}

Quaternion Quaternion::operator-(const Quaternion& rhs) const {
  return Quaternion(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
}

void Quaternion::operator-=(const Quaternion& rhs) {
  x -= rhs.x, y -= rhs.y, z -= rhs.z, w -= rhs.w;
}

Quaternion Quaternion::operator/(const Quaternion& rhs) const {
  return Quaternion(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w);
}

void Quaternion::operator/=(const Quaternion& rhs) {
  x /= rhs.x, y /= rhs.y, z /= rhs.z, w /= rhs.w;
}

Quaternion Quaternion::operator/(const float rhs) const {
  return Quaternion(x / rhs, y / rhs, z / rhs, w / rhs);
}

void Quaternion::operator/=(const float rhs) {
  x /= rhs, y /= rhs, z /= rhs, w /= rhs;
}

bool Quaternion::operator!=(const Quaternion& rhs) const {
  return x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w;
}

bool Quaternion::operator==(const Quaternion& rhs) const {
  return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
}

float Quaternion::Dot(const Quaternion& quat) const {
  return x * x + y * y + z * z + w * w;
}

void Quaternion::FromAngle(const Vector3& vec) {
  float t0 = std::cos(vec[2] * 0.5f);
  float t1 = std::sin(vec[2] * 0.5f);
  float t2 = std::cos(vec[0] * 0.5f);
  float t3 = std::sin(vec[0] * 0.5f);
  float t4 = std::cos(vec[1] * 0.5f);
  float t5 = std::sin(vec[1] * 0.5f);

  w = t0 * t2 * t4 + t1 * t3 * t5;
  x = t0 * t3 * t4 - t1 * t2 * t5;
  y = t0 * t2 * t5 + t1 * t3 * t4;
  z = t1 * t2 * t4 - t0 * t3 * t5;
}

void Quaternion::FromAxisAngle(Vector3 axis, float angle) {
  float const s = std::sin(angle * 0.5f);
  w = std::cos(angle * 0.5f);
  x = axis[0] * s;
  y = axis[1] * s;
  z = axis[2] * s;
}

void Quaternion::GetAngles(Vector3& vec) {
  float ysqr = y * y;

  float t0 = 2.0f * (w * x + y * z);
  float t1 = 1.0f - 2.0f * (x * x + ysqr);
  vec[0] = std::atan2(t0, t1);

  float t2 = 2.0f * (w * y - z * x);
  t2 = t2 > 1.0f ? 1.0f : t2;
  t2 = t2 < -1.0f ? -1.0f : t2;
  vec[1] = std::asin(t2);

  float t3 = +2.0f * (w * z + x * y);
  float t4 = +1.0f - 2.0f * (ysqr + z * z);
  vec[2] = std::atan2(t3, t4);
}

void Quaternion::Slerp(const Quaternion& quat, float factor) {
  Quaternion q3;
  float dot = Dot(quat);
  if (dot < 0) {
    dot = -dot;
    q3 = -quat;
  } else
    q3 = quat;

  float angle = acosf(dot);
  *this = (*this * sinf(angle * (1 - factor)) + q3 * sinf(angle * factor)) /
          sinf(angle);
}

Quaternion Quaternion::Slerp(const Quaternion& quat1, const Quaternion& quat2,
                             const float factor) {
  Quaternion q3;
  float dot = quat1.Dot(quat2);
  if (dot < 0) {
    dot = -dot;
    q3 = -quat2;
  } else
    q3 = quat2;

  float angle = acosf(dot);
  return (quat1 * sinf(angle * (1 - factor)) + q3 * sinf(angle * factor)) /
         sinf(angle);
}

void Quaternion::RotationBetweenVectors(Vector3 vec1, Vector3 vec2) {
  vec1.Normalize();
  vec2.Normalize();

  float cos_theta = vec1.Dot(vec2);
  Vector3 rotation_axis;

  if (cos_theta < -1 + 0.001f) {
    rotation_axis = Vector3(0.0f, 0.0f, 1.0f).Cross(vec1);
    if (rotation_axis.Length() < 0.01)
      rotation_axis = Vector3(1.0f, 0.0f, 0.0f).Cross(vec1);

    rotation_axis.Normalize();
    FromAxisAngle(rotation_axis, PI);
    return;
  }

  rotation_axis = vec1.Cross(vec2);

  float s = std::sqrt((1 + cos_theta) * 2);
  float invs = 1 / s;

  w = s * 0.5f;
  x = rotation_axis[0] * invs;
  y = rotation_axis[1] * invs;
  z = rotation_axis[2] * invs;
}

void Quaternion::RotateVector(Vector3& vec) {
  Vector3 u(x, y, z);
  vec = u * 2.0f * u.Dot(vec) + vec * (w * w - u.Dot(u)) +
        u.Cross(vec) * 2.0f * w;
}

void Quaternion::Normalize() {
  float n = std::sqrt(x * x + y * y + z * z + w * w);
  x /= n, y /= n, z /= n, w /= n;
}

Matrix4x4 Quaternion::RotationMatrix() {
  Matrix4x4 rot_1, rot_2;
  rot_1.data[0] = rot_1.data[5] = rot_1.data[10] = rot_1.data[15] =
      rot_2.data[0] = rot_2.data[5] = rot_2.data[10] = rot_2.data[15] = w;

  rot_1.data[4] = rot_1.data[14] = rot_2.data[4] = rot_2.data[11] = z;
  rot_1.data[1] = rot_1.data[11] = rot_2.data[1] = rot_2.data[14] = -z;

  rot_1.data[12] = rot_1.data[9] = rot_2.data[3] = rot_2.data[9] = x;
  rot_1.data[3] = rot_1.data[6] = rot_2.data[6] = rot_2.data[12] = -x;

  rot_1.data[2] = rot_1.data[13] = rot_2.data[2] = rot_2.data[7] = y;
  rot_1.data[7] = rot_1.data[8] = rot_2.data[8] = rot_2.data[13] = -y;

  rot_1 *= rot_2;
  return rot_1;
}
}  // namespace lib_core