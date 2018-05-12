#pragma once
#include <functional>
#include <typeindex>
#include <unordered_map>
#include "any_type.hpp"

namespace lib_core {
class Factory {
 public:
  template <typename T, typename... Args>
  void add_creator(std::function<T(Args...)> create_func) {
    creation_functions_[std::type_index(typeid(T))] = any_type(create_func);
  }

  template <typename T, typename... Args>
  T create(Args... vals) {
    auto itr = creation_functions_.find(std::type_index(typeid(T)));
    if (itr == creation_functions_.end()) return nullptr;
    return itr->second.get_value<std::function<T(Args...)>>()(vals...);
  }

 private:
  std::unordered_map<std::type_index, any_type> creation_functions_;
};
}  // namespace lib_core
