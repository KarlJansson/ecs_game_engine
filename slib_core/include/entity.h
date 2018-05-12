#pragma once
#include <functional>

namespace lib_core {
class Entity {
 public:
  Entity();
  explicit Entity(size_t id);
  ~Entity() = default;

  size_t operator()() { return id_; }
  bool operator==(const Entity& rhs) const { return id_ == rhs.id_; }
  bool operator!=(const Entity& rhs) const { return !operator==(rhs); }
  bool operator<(const Entity& rhs) const { return id_ < rhs.id_; }
  bool operator>(const Entity& rhs) const { return operator<(rhs); }
  bool operator<=(const Entity& rhs) const { return !operator>(rhs); }
  bool operator>=(const Entity& rhs) const { return !operator<(rhs); }

  size_t id_;
};
}  // namespace lib_core

namespace std {
template <>
struct hash<lib_core::Entity> {
  size_t operator()(const lib_core::Entity& x) const {
    return hash<size_t>()(x.id_);
  }
};
}  // namespace std
