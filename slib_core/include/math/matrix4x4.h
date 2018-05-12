#pragma once

namespace lib_core {
class Matrix4x4 {
 public:
  Matrix4x4() = default;
  ~Matrix4x4() = default;

  Matrix4x4 operator*(const Matrix4x4& rhs);
  void operator*=(const Matrix4x4& rhs);

  void Forward(class Vector3& vec) const;
  void Left(class Vector3& vec) const;
  void Up(class Vector3& vec) const;
  void Translation(class Vector3& vec) const;

  void RotationMatrix(class Matrix3x3& rot) const;
  void Translate(const class Vector3& vec);
  void Scale(const class Vector3& vec);

  void Identity();
  void Inverse();
  void Transpose();

  void Lookat(class Vector3 eye, class Vector3 center, class Vector3 up);
  void Perspective(float fovy, float aspect, float znear, float zfar);
  void Orthographic(float left, float right, float bottom, float top,
                    float znear, float zfar);
  void Orthographic(float left, float right, float bottom, float top);

  float data[16];
};
}  // namespace lib_core
