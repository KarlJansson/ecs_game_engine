#pragma once

namespace lib_core {
class Matrix3x3 {
 public:
  Matrix3x3() = default;
  ~Matrix3x3() = default;

  Matrix3x3 operator*(const Matrix3x3& rhs);
  void operator*=(const Matrix3x3& rhs);

  float data[9];
};
}  // namespace lib_core
