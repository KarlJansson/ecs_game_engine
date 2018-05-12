#pragma once
#include <memory>

struct any_type {
  template <typename T>
  explicit any_type(T t) : ptr(std::make_shared<impl<T>>(t)) {}
  any_type() : ptr() {}
  ~any_type() = default;

  template <typename T>
  T& get_value() const {
    return std::static_pointer_cast<impl<T>>(ptr)->get_value();
  }

 private:
  struct placeholder {
    virtual ~placeholder() {}
  };

  template <typename T>
  struct impl : placeholder {
    explicit impl(T t) : val(t) {}
    T& get_value() { return val; }
    T val;
  };

  std::shared_ptr<placeholder> ptr;
};
