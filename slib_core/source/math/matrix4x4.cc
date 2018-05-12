#include "matrix4x4.h"
#include <cmath>
#include "vector_def.h"

namespace lib_core {
Matrix4x4 Matrix4x4::operator*(const Matrix4x4& rhs) {
  Matrix4x4 res;
  res.data[0] = data[0] * rhs.data[0] + data[4] * rhs.data[1] +
                data[8] * rhs.data[2] + data[12] * rhs.data[3];
  res.data[4] = data[0] * rhs.data[4] + data[4] * rhs.data[5] +
                data[8] * rhs.data[6] + data[12] * rhs.data[7];
  res.data[8] = data[0] * rhs.data[8] + data[4] * rhs.data[9] +
                data[8] * rhs.data[10] + data[12] * rhs.data[11];
  res.data[12] = data[0] * rhs.data[12] + data[4] * rhs.data[13] +
                 data[8] * rhs.data[14] + data[12] * rhs.data[15];

  res.data[1] = data[1] * rhs.data[0] + data[5] * rhs.data[1] +
                data[9] * rhs.data[2] + data[13] * rhs.data[3];
  res.data[5] = data[1] * rhs.data[4] + data[5] * rhs.data[5] +
                data[9] * rhs.data[6] + data[13] * rhs.data[7];
  res.data[9] = data[1] * rhs.data[8] + data[5] * rhs.data[9] +
                data[9] * rhs.data[10] + data[13] * rhs.data[11];
  res.data[13] = data[1] * rhs.data[12] + data[5] * rhs.data[13] +
                 data[9] * rhs.data[14] + data[13] * rhs.data[15];

  res.data[2] = data[2] * rhs.data[0] + data[6] * rhs.data[1] +
                data[10] * rhs.data[2] + data[14] * rhs.data[3];
  res.data[6] = data[2] * rhs.data[4] + data[6] * rhs.data[5] +
                data[10] * rhs.data[6] + data[14] * rhs.data[7];
  res.data[10] = data[2] * rhs.data[8] + data[6] * rhs.data[9] +
                 data[10] * rhs.data[10] + data[14] * rhs.data[11];
  res.data[14] = data[2] * rhs.data[12] + data[6] * rhs.data[13] +
                 data[10] * rhs.data[14] + data[14] * rhs.data[15];

  res.data[3] = data[3] * rhs.data[0] + data[7] * rhs.data[1] +
                data[11] * rhs.data[2] + data[15] * rhs.data[3];
  res.data[7] = data[3] * rhs.data[4] + data[7] * rhs.data[5] +
                data[11] * rhs.data[6] + data[15] * rhs.data[7];
  res.data[11] = data[3] * rhs.data[8] + data[7] * rhs.data[9] +
                 data[11] * rhs.data[10] + data[15] * rhs.data[11];
  res.data[15] = data[3] * rhs.data[12] + data[7] * rhs.data[13] +
                 data[11] * rhs.data[14] + data[15] * rhs.data[15];

  return res;
}

void Matrix4x4::operator*=(const Matrix4x4& rhs) { *this = *this * rhs; }

void Matrix4x4::Forward(Vector3& vec) const {
  vec[0] = data[8], vec[1] = data[9], vec[2] = data[10];
}

void Matrix4x4::Left(Vector3& vec) const {
  vec[0] = data[0], vec[1] = data[1], vec[2] = data[2];
}

void Matrix4x4::Up(Vector3& vec) const {
  vec[0] = data[4], vec[1] = data[5], vec[2] = data[6];
}

void Matrix4x4::Translation(Vector3& vec) const {
  vec[0] = data[12];
  vec[1] = data[13];
  vec[2] = data[14];
}

void Matrix4x4::RotationMatrix(Matrix3x3& rot) const {
  rot.data[0] = data[0], rot.data[1] = data[1], rot.data[2] = data[2];

  rot.data[3] = data[4], rot.data[4] = data[5], rot.data[5] = data[6];

  rot.data[6] = data[8], rot.data[7] = data[9], rot.data[8] = data[10];
}

void Matrix4x4::Translate(const Vector3& vec) {
  data[12] += vec[0], data[13] += vec[1], data[14] += vec[2];
}

void Matrix4x4::Scale(const Vector3& vec) {
  data[0] *= vec[0], data[1] *= vec[0], data[2] *= vec[0], data[3] *= vec[0];
  data[4] *= vec[1], data[5] *= vec[1], data[6] *= vec[1], data[7] *= vec[1];
  data[8] *= vec[2], data[9] *= vec[2], data[10] *= vec[2], data[11] *= vec[2];
}

void Matrix4x4::Identity() {
  data[0] = data[5] = data[10] = data[15] = 1.0f;
  data[1] = data[2] = data[3] = data[4] = data[6] = data[7] = data[8] =
      data[9] = data[11] = data[12] = data[13] = data[14] = 0.0f;
}

void Matrix4x4::Inverse() {
  float inv[16], det;
  auto m = data;

  inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] +
           m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
  inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] -
           m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
  inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] +
           m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
  inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] -
            m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
  inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] -
           m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
  inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] +
           m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
  inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] -
           m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
  inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] +
            m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
  inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] +
           m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
  inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] -
           m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
  inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] +
            m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
  inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] -
            m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
  inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] -
           m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
  inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] +
           m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
  inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] -
            m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
  inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] +
            m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

  det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
  det = 1.0f / det;

  for (int i = 0; i < 16; i++) m[i] = inv[i] * det;
}

void Matrix4x4::Transpose() {
  float tmp;
  tmp = data[1];
  data[1] = data[4];
  data[4] = tmp;

  tmp = data[2];
  data[2] = data[8];
  data[8] = tmp;

  tmp = data[3];
  data[3] = data[12];
  data[12] = tmp;

  tmp = data[6];
  data[6] = data[9];
  data[9] = tmp;

  tmp = data[7];
  data[7] = data[13];
  data[13] = tmp;

  tmp = data[11];
  data[11] = data[14];
  data[14] = tmp;
}

void Matrix4x4::Lookat(Vector3 eye, Vector3 center, Vector3 up) {
  center -= eye;
  center.Normalize();
  up = center.Cross(up);
  up.Normalize();
  auto u = up.Cross(center);

  data[0] = up[0], data[4] = up[1], data[8] = up[2];
  data[1] = u[0], data[5] = u[1], data[9] = u[2];
  data[2] = -center[0], data[6] = -center[1], data[10] = -center[2];
  data[12] = -up.Dot(eye), data[13] = -u.Dot(eye), data[14] = center.Dot(eye);

  data[3] = data[7] = data[11] = 0.0f;
  data[15] = 1.0f;
}

void Matrix4x4::Perspective(float fovy, float aspect, float znear, float zfar) {
  float const tanHalfFovy = std::tan(fovy * .5f);

  data[1] = data[2] = data[3] = data[4] = data[6] = data[7] = data[8] =
      data[9] = data[12] = data[13] = data[15] = 0.0f;

  data[0] = 1.0f / (aspect * tanHalfFovy);
  data[5] = 1.0f / tanHalfFovy;
  data[11] = -1.0f;
  data[10] = -(zfar + znear) / (zfar - znear);
  data[14] = -(2.0f * zfar * znear) / (zfar - znear);
}

void Matrix4x4::Orthographic(float left, float right, float bottom, float top,
                             float znear, float zfar) {
  data[1] = data[2] = data[3] = data[4] = data[6] = data[7] = data[8] =
      data[9] = data[11] = 0.0f;
  data[15] = 1.0f;

  data[0] = 2.0f / (right - left);
  data[5] = 2.0f / (top - bottom);
  data[12] = -(right + left) / (right - left);
  data[13] = -(top + bottom) / (top - bottom);

  data[10] = -2.0f / (zfar - znear);
  data[14] = -(zfar + znear) / (zfar - znear);
}

void Matrix4x4::Orthographic(float left, float right, float bottom, float top) {
  data[1] = data[2] = data[3] = data[4] = data[6] = data[7] = data[8] =
      data[9] = data[11] = data[14] = 0.0f;

  data[15] = 1.0f;
  data[0] = 2.0f / (right - left);
  data[5] = 2.0f / (top - bottom);
  data[10] = -1.0f;
  data[12] = -(right + left) / (right - left);
  data[13] = -(top + bottom) / (top - bottom);
}
}  // namespace lib_core