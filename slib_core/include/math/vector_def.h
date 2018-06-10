#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include "matrix3x3.h"
#include "matrix4x4.h"

namespace lib_core {
template <uint8_t I, typename T>
class TVector {
 public:
  TVector() = default;
  ~TVector() = default;

  T& operator[](size_t idx) { return data_[idx]; }

  const T operator[](size_t idx) const { return data_[idx]; }

  TVector<I, T> operator*(const TVector<I, T>& rhs) const {
    TVector<I, T> out;
    for (uint8_t i = 0; i < I; ++i) out.data_[i] = data_[i] * rhs.data_[i];
    return out;
  }

  void operator*=(const TVector<I, T>& rhs) {
    for (uint8_t i = 0; i < I; ++i) data_[i] *= rhs.data_[i];
  }

  TVector<I, T> operator+(const TVector<I, T>& rhs) const {
    TVector<I, T> out;
    for (uint8_t i = 0; i < I; ++i) out.data_[i] = data_[i] + rhs.data_[i];
    return out;
  }

  void operator+=(const TVector<I, T>& rhs) {
    for (auto i = 0; i < I; ++i) data_[i] += rhs.data_[i];
  }

  TVector<I, T> operator-(const TVector<I, T>& rhs) const {
    TVector<I, T> out;
    for (uint8_t i = 0; i < I; ++i) out.data_[i] = data_[i] - rhs.data_[i];
    return out;
  }

  void operator-=(const TVector<I, T>& rhs) {
    for (auto i = 0; i < I; ++i) data_[i] -= rhs.data_[i];
  }

  TVector<I, T> operator/(const TVector<I, T>& rhs) const {
    TVector<I, T> out;
    for (uint8_t i = 0; i < I; ++i) out.data_[i] = data_[i] / rhs.data_[i];
    return out;
  }

  void operator/=(const TVector<I, T>& rhs) {
    for (uint8_t i = 0; i < I; ++i) data_[i] /= rhs.data_[i];
  }

  TVector<I, T> operator*(const T rhs) const {
    TVector<I, T> out;
    for (uint8_t i = 0; i < I; ++i) out.data_[i] = data_[i] * rhs;
    return out;
  }

  void operator*=(const T rhs) {
    for (uint8_t i = 0; i < I; ++i) data_[i] *= rhs;
  }

  TVector<I, T> operator+(const T rhs) const {
    TVector<I, T> out;
    for (uint8_t i = 0; i < I; ++i) out.data_[i] = data_[i] + rhs;
    return out;
  }

  void operator+=(const T rhs) {
    for (uint8_t i = 0; i < I; ++i) data_[i] += rhs;
  }

  TVector<I, T> operator-(const T rhs) const {
    TVector<I, T> out;
    for (uint8_t i = 0; i < I; ++i) out.data_[i] = data_[i] - rhs;
    return out;
  }

  void operator-=(const T rhs) {
    for (uint8_t i = 0; i < I; ++i) data_[i] -= rhs;
  }

  TVector<I, T> operator/(const T rhs) const {
    TVector<I, T> out;
    for (uint8_t i = 0; i < I; ++i) out.data_[i] = data_[i] / rhs;
    return out;
  }

  void operator/=(const T rhs) {
    for (uint8_t i = 0; i < I; ++i) data_[i] /= rhs;
  }

  bool operator==(const TVector<I, T>& rhs) const {
    for (uint8_t i = 0; i < I; ++i)
      if (data_[i] != rhs[i]) return false;
    return true;
  }

  bool operator!=(const TVector<I, T>& rhs) const { return !(*this == rhs); }

  T Length() const {
    auto val = T(.0);
    for (uint8_t i = 0; i < I; ++i) val += data_[i] * data_[i];
    return sqrt(val);
  }

  bool Zero() const {
    for (uint8_t i = 0; i < I; ++i)
      if (data_[i] != T(.0)) return false;
    return true;
  }

  void ZeroMem() { std::memset(data_, 0, sizeof(T) * I); }

  void Normalize() {
    T l = Length();
    for (uint8_t i = 0; i < I; ++i) data_[i] /= l;
  }

  T Dot(const TVector<I, T>& rhs) const {
    T val = .0f;
    for (uint8_t i = 0; i < I; ++i) val += data_[i] * rhs.data_[i];
    return val;
  }

  T Angle(const TVector<I, T>& rhs) const {
    auto prod_1 = *this * rhs.Length();
    auto prod_2 = rhs * this->Length();
    auto length = (prod_1 + prod_2).Length();
    if (length == T(.0)) return T(.0);

    return T(2.) * atan((prod_1 - prod_2).Length() / length);
  }

  TVector<I, T> Midpoint(const TVector<I, T>& rhs) const {
    TVector<I, T> out;
    for (uint8_t i = 0; i < I; ++i)
      out.data_[i] = (data_[i] + rhs.data_[i]) * T(.5);
    return out;
  }

 protected:
  T data_[I];
};

class Vector2 : public TVector<2, float> {
 public:
  Vector2() = default;
  Vector2(float val) { data_[0] = val, data_[1] = val; }
  Vector2(float x, float y) { data_[0] = x, data_[1] = y; }
  Vector2(const TVector<2, float>& init) {
    for (int i = 0; i < 2; ++i) data_[i] = init[i];
  }
  ~Vector2() = default;
};

class Vector3 : public TVector<3, float> {
 public:
  Vector3() = default;
  Vector3(float val) { data_[0] = val, data_[1] = val, data_[2] = val; }
  Vector3(float x, float y, float z) {
    data_[0] = x, data_[1] = y, data_[2] = z;
  }
  Vector3(const TVector<3, float>& init) {
    for (int i = 0; i < 3; ++i) data_[i] = init[i];
  }
  ~Vector3() = default;

  Vector3 Cross(const Vector3& rhs);
  void Transform(const Matrix3x3& matrix);
  float Transform(const Matrix4x4& matrix);
};

class Vector4 : public TVector<4, float> {
 public:
  Vector4() = default;
  Vector4(float val) {
    data_[0] = val, data_[1] = val, data_[2] = val, data_[3] = val;
  }
  Vector4(float x, float y, float z, float w) {
    data_[0] = x, data_[1] = y, data_[2] = z, data_[3] = w;
  }
  Vector4(const TVector<4, float>& init) {
    for (int i = 0; i < 4; ++i) data_[i] = init[i];
  }
  ~Vector4() = default;
};

}  // namespace lib_core
