#pragma once
#include "core_utilities.h"

template <typename T>
class storage_array {
 public:
  size_t add(T item) {
    ++size_;
    if (open_storage_slots_.empty()) {
      storage_.emplace_back(std::move(item));
      return storage_.size() - 1;
    } else {
      auto slot = *open_storage_slots_.begin();
      storage_[slot] = std::move(item);
      open_storage_slots_.erase(open_storage_slots_.begin());
      return slot;
    }
  }

  void erase(size_t id) {
    open_storage_slots_.insert(id);
    storage_[id] = T();
    --size_;
  }

  T& operator[](size_t i) { return storage_[i]; }
  const T& operator[](size_t i) const { return storage_[i]; }

  T* get_ordered(size_t i) {
    auto i_mod = i;
    for (auto& s : open_storage_slots_)
      if (s <= i)
        i_mod++;
      else
        break;

    if (i_mod >= storage_.size()) return nullptr;
    return &storage_[i_mod];
  }

  void iterate(std::function<bool(T&, size_t i)> func) {
    if (open_storage_slots_.empty()) {
      for (size_t i = 0; i < storage_.size(); ++i)
        if (!func(storage_[i], i)) break;
    } else {
      size_t i = 0;
      bool stop_iterating = false;
      for (auto& s : open_storage_slots_) {
        for (; i < s; ++i) {
          if (!func(storage_[i], i)) {
            stop_iterating = true;
            break;
          }
        }
        if (stop_iterating) break;
        ++i;
      }
    }
  }

  void iterate(std::function<bool(T&)> func) {
    if (open_storage_slots_.empty()) {
      for (size_t i = 0; i < storage_.size(); ++i)
        if (!func(storage_[i])) break;
    } else {
      size_t i = 0;
      bool stop_iterating = false;
      for (auto& s : open_storage_slots_) {
        for (; i < s; ++i) {
          if (!func(storage_[i])) {
            stop_iterating = true;
            break;
          }
        }
        if (stop_iterating) break;
        ++i;
      }
    }
  }

  size_t size() { return size_; }

 private:
  ct::tree_set<size_t> open_storage_slots_;
  ct::dyn_array<T> storage_;
  size_t size_ = 0;
};