#pragma once

class range {
 public:
  range(size_t min, size_t max) : min_(min), max_(max) {}

  class iterator {
   public:
    using difference_type =
        typename std::iterator<std::random_access_iterator_tag,
                               size_t>::difference_type;

    iterator() : val_(0) {}
    iterator(size_t rhs) : val_(rhs) {}
    iterator(const iterator& rhs) : val_(rhs.val_) {}

    iterator& operator+=(difference_type rhs) {
      val_ += rhs;
      return *this;
    }
    iterator& operator-=(difference_type rhs) {
      val_ -= rhs;
      return *this;
    }

    size_t& operator*() { return val_; }
    size_t* operator->() { return &val_; }
    size_t operator[](difference_type rhs) { return val_ + rhs; }

    iterator& operator++() {
      ++val_;
      return *this;
    }
    iterator& operator--() {
      --val_;
      return *this;
    }
    iterator operator++(int) {
      iterator tmp(*this);
      ++val_;
      return tmp;
    }
    iterator operator--(int) {
      iterator tmp(*this);
      --val_;
      return tmp;
    }
    difference_type operator-(const iterator& rhs) const {
      return val_ - rhs.val_;
    }
    iterator operator+(difference_type rhs) const {
      return iterator(val_ + rhs);
    }
    iterator operator-(difference_type rhs) const {
      return iterator(val_ - rhs);
    }

    bool operator==(const iterator& rhs) const { return val_ == rhs.val_; }
    bool operator!=(const iterator& rhs) const { return val_ != rhs.val_; }
    bool operator>(const iterator& rhs) const { return val_ > rhs.val_; }
    bool operator<(const iterator& rhs) const { return val_ < rhs.val_; }
    bool operator>=(const iterator& rhs) const { return val_ >= rhs.val_; }
    bool operator<=(const iterator& rhs) const { return val_ <= rhs.val_; }

   private:
    size_t val_;
  };

  iterator begin() { return iterator(min_); }
  iterator end() { return iterator(max_); }

  size_t min_;
  size_t max_;
};
