#pragma once
#include "core_utilities.h"

template <class... Types>
class struct_of_arrays {
 public:
  template <size_t I, typename T>
  std::vector<T>& get() {
    return std::get<I>(arrays_);
  }

  template <size_t I>
  size_t size() const {
    return std::get<I>(arrays_).size();
  }

  void emplace_back(Types... vals) {
    emplace_back_rec(std::make_tuple(vals...));
  }
  void pop_back() { pop_back_rec(arrays_); }
  void shrink_to_size() { shrink_to_size_rec(arrays_); }
  void erase(size_t i) { erase_rec(i, arrays_); }
  void reserve(size_t nr) { reserve_rec(nr, arrays_); }
  void clear() { clear_rec(arrays_); }

 private:
  std::tuple<ct::dyn_array<Types>...> arrays_;

  template <std::size_t I = 0, typename... funcTypes>
  inline typename std::enable_if<I == sizeof...(funcTypes), void>::type
  emplace_back_rec(std::tuple<funcTypes...>&) {}
  template <std::size_t I = 0, typename... funcTypes>
      inline typename std::enable_if <
      I<sizeof...(funcTypes), void>::type emplace_back_rec(
          std::tuple<funcTypes...>& t) {
    std::get<I>(arrays_).emplace_back(std::move(std::get<I>(t)));
    emplace_back_rec<I + 1, funcTypes...>(t);
  }

  template <std::size_t I = 0, typename... funcTypes>
  inline typename std::enable_if<I == sizeof...(funcTypes), void>::type
  pop_back_rec(std::tuple<funcTypes...>&) {}
  template <std::size_t I = 0, typename... funcTypes>
      inline typename std::enable_if <
      I<sizeof...(funcTypes), void>::type pop_back_rec(std::tuple<funcTypes...>& t) {
    std::get<I>(t).pop_back();
    pop_back_rec<I + 1, Types...>(t);
  }

  template <std::size_t I = 0, typename... funcTypes>
  inline typename std::enable_if<I == sizeof...(funcTypes), void>::type erase_rec(
      size_t i, std::tuple<funcTypes...>& t) {}
  template <std::size_t I = 0, typename... funcTypes>
      inline typename std::enable_if <
      I<sizeof...(funcTypes), void>::type erase_rec(size_t i,
                                                std::tuple<funcTypes...>& t) {
    auto& a = std::get<I>(t);
    a.erase(a.begin() + i);
    erase_rec<I + 1, funcTypes...>(i, t);
  }

  template <std::size_t I = 0, typename... funcTypes>
  inline typename std::enable_if<I == sizeof...(funcTypes), void>::type
  reserver_rec(size_t i, std::tuple<funcTypes...>& t) {}
  template <std::size_t I = 0, typename... funcTypes>
      inline typename std::enable_if <
      I<sizeof...(funcTypes), void>::type reserver_rec(size_t i,
                                                   std::tuple<funcTypes...>& t) {
    std::get<I>(t).reserve(i);
    reserver_rec<I + 1, funcTypes...>(i, t);
  }

  template <std::size_t I = 0, typename... funcTypes>
  inline typename std::enable_if<I == sizeof...(funcTypes), void>::type
  shrink_to_size_rec(std::tuple<funcTypes...>&) {}
  template <std::size_t I = 0, typename... funcTypes>
      inline typename std::enable_if <
      I<sizeof...(funcTypes), void>::type shrink_to_size_rec(
          std::tuple<funcTypes...>& t) {
    std::get<I>(t).shrink_to_size();
    shrink_to_size_rec<I + 1, funcTypes...>(t);
  }

  template <std::size_t I = 0, typename... funcTypes>
  inline typename std::enable_if<I == sizeof...(funcTypes), void>::type clear_rec(
      std::tuple<funcTypes...>&) {}
  template <std::size_t I = 0, typename... funcTypes>
      inline typename std::enable_if <
      I<sizeof...(funcTypes), void>::type clear_rec(std::tuple<funcTypes...>& t) {
    std::get<I>(t).clear();
    clear_rec<I + 1, funcTypes...>(t);
  }
};
