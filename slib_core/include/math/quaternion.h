#pragma once
#include "matrix4x4.h"
#include "vector_def.h"

namespace lib_core {
class Quaternion {
 public:
  Quaternion() = default;
  Quaternion(float x, float y, float z, float w) {
    this->x = x, this->y = y, this->z = z, this->w = w;
  }
  ~Quaternion() = default;

  const Quaternion operator-() const;

  Quaternion operator*(const Quaternion& rhs) const;
  void operator*=(const Quaternion& rhs);

  Quaternion operator*(const float rhs) const;
  void operator*=(const float rhs);

  Quaternion operator+(const Quaternion& rhs) const;
  void operator+=(const Quaternion& rhs);

  Quaternion operator-(const Quaternion& rhs) const;
  void operator-=(const Quaternion& rhs);

  Quaternion operator/(const Quaternion& rhs) const;
  void operator/=(const Quaternion& rhs);

  Quaternion operator/(const float rhs) const;
  void operator/=(const float rhs);

  bool operator!=(const Quaternion& rhs) const;
  bool operator==(const Quaternion& rhs) const;

  float Dot(const Quaternion& quat) const;

  void FromAngle(const Vector3& vec);
  void FromAxisAngle(const Vector3 axis, const float angle);

  void GetAngles(Vector3& vec);

  void Slerp(const Quaternion& quat, const float factor);
  Quaternion Slerp(const Quaternion& quat1, const Quaternion& quat2,
                   const float factor);
  void RotationBetweenVectors(Vector3 vec1, Vector3 vec2);

  void RotateVector(Vector3& vec);

  void Normalize();
  Matrix4x4 RotationMatrix();

  float x = 0;
  float y = 0;
  float z = 0;
  float w = 1;
};
}  // namespace lib_core
