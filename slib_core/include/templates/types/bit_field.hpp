#pragma once
#include <functional>
#include <vector>

template <typename T>
class bit_field {
 public:
  explicit bit_field(size_t blocks = 10) { bit_field_.reserve(blocks); }

  void push_back(bool val) {
    ++size_;
    auto block = (size_ / block_size_) + 1;
    if (block >= bit_field_.size()) {
      bit_field_.push_back({1});
      return;
    }

    auto block_pos = size_ % block_size_;
    bit_field_.back() |= 1 << block_pos;
  }

  void assign(size_t count, T val = 0) {
    size_ = count;
    auto blocks = (size_ / block_size_) + 1;
    bit_field_.assign(blocks, val > 0 ? max_value_ : 0);
  }

  size_t count_set() {
    size_t count = 0;
    size_t index = 0, sub_index;
    for (auto b : bit_field_) {
      if (b == max_value_) {
        count += block_size_;
      } else if (b != 0) {
        sub_index = 0;
        while (b != 0) {
          if (b & 1) count += 1;
          b >>= 1;
          ++sub_index;
          if (index + sub_index >= size_) return count;
        }
      }
      index += block_size_;
    }
    return count > size_ ? size_ : count;
  }

  std::vector<size_t> get_set() {
    std::vector<size_t> inds;
    size_t index = 0, sub_index;
    for (auto b : bit_field_) {
      if (b == max_value_) {
        for (auto i = 0; i < block_size_; ++i) {
          inds.emplace_back(index + i);
          if (index + i >= size_) return inds;
        }
      } else if (b != 0) {
        sub_index = 0;
        while (b != 0) {
          if (b & 1) inds.emplace_back(index + sub_index);
          b >>= 1;
          ++sub_index;
          if (index + sub_index >= size_) return inds;
        }
      }
      index += block_size_;
    }
    return inds;
  }

  void call_if_set(std::function<void(size_t)> func) {
    size_t index = 0, sub_index;
    for (auto b : bit_field_) {
      if (b == max_value_) {
        for (auto i = 0; i < block_size_; ++i) {
          func(index + i);
          if (index + i >= size_) return;
        }
      } else if (b != 0) {
        sub_index = 0;
        while (b != 0) {
          if (b & 1) func(index + sub_index);
          b >>= 1;
          ++sub_index;
          if (index + sub_index >= size_) return;
        }
      }
      index += block_size_;
    }
  }

  bool is_set(size_t pos) {
    if (pos >= size_) return false;

    auto block = pos / block_size_;
    if (block == 0) return false;
    if (block == max_value_) return true;

    auto block_pos = pos % block_size_;
    return bit_field_[block] & block_pos;
  }

  void set(size_t pos) {
    if (pos >= size_) return;

    auto block = pos / block_size_;
    auto block_pos = pos % block_size_;
    bit_field_[block] |= (1 << block_pos);
  }

  void unset(size_t pos) {
    if (!is_set()) return;

    auto block = pos / block_size_;
    auto block_pos = pos % block_size_;
    bit_field_[block] ^= (1 << block_pos);
  }

  size_t size() { return size_; }

 private:
  size_t size_{0};
  std::vector<T> bit_field_;

  static constexpr T max_value_ = ~T{0};
  static constexpr size_t block_size_{sizeof(T) * 8};
};
