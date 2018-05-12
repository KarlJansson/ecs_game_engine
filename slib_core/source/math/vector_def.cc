#include "vector_def.h"

namespace lib_core {
Vector3 Vector3::Cross(const Vector3& rhs) {
  return Vector3(data_[1] * rhs.data_[2] - data_[2] * rhs.data_[1],
                 data_[2] * rhs.data_[0] - data_[0] * rhs.data_[2],
                 data_[0] * rhs.data_[1] - data_[1] * rhs.data_[0]);
}

void Vector3::Transform(const Matrix3x3& matrix) {
  float tmp[3] = {data_[0], data_[1], data_[2]};
  data_[0] = matrix.data[0] * tmp[0] + matrix.data[3] * tmp[1] +
             matrix.data[6] * tmp[2];
  data_[1] = matrix.data[1] * tmp[0] + matrix.data[4] * tmp[1] +
             matrix.data[7] * tmp[2];
  data_[2] = matrix.data[2] * tmp[0] + matrix.data[5] * tmp[1] +
             matrix.data[8] * tmp[2];
}

float Vector3::Transform(const Matrix4x4& matrix) {
  float tmp[3] = {data_[0], data_[1], data_[2]};
  data_[0] = matrix.data[0] * tmp[0] + matrix.data[4] * tmp[1] +
             matrix.data[8] * tmp[2] + matrix.data[12];
  data_[1] = matrix.data[1] * tmp[0] + matrix.data[5] * tmp[1] +
             matrix.data[9] * tmp[2] + matrix.data[13];
  data_[2] = matrix.data[2] * tmp[0] + matrix.data[6] * tmp[1] +
             matrix.data[10] * tmp[2] + matrix.data[14];
  return matrix.data[3] * tmp[0] + matrix.data[7] * tmp[1] +
         matrix.data[11] * tmp[2] + matrix.data[15];
}
}  // namespace lib_core
