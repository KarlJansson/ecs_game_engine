#include "matrix3x3.h"

namespace lib_core {
Matrix3x3 Matrix3x3::operator*(const Matrix3x3& rhs) { return Matrix3x3(); }
void Matrix3x3::operator*=(const Matrix3x3& rhs) {}
}  // namespace lib_core