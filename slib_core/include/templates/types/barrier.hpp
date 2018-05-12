#pragma once
#include <condition_variable>
#include <mutex>

class Barrier {
 public:
  explicit Barrier(int num_threads)
      : num_threads_(num_threads), original_threads_(num_threads) {
    condition_ = std::make_unique<std::condition_variable>();
    mutex_ = std::make_unique<std::mutex>();
  }

  void Wait() {
    std::unique_lock<std::mutex> lk(*mutex_);
    --num_threads_;
    if (num_threads_ <= 0) {
      num_threads_ = original_threads_;
      lk.unlock();
      condition_->notify_all();
    } else {
      condition_->wait(lk);
    }
  }

  void Signal() {
    std::unique_lock<std::mutex> lk(*mutex_);
    --num_threads_;
    if (num_threads_ <= 0) {
      num_threads_ = original_threads_;
      lk.unlock();
      condition_->notify_all();
    }
  }

 private:
  std::unique_ptr<std::condition_variable> condition_;
  std::unique_ptr<std::mutex> mutex_;

  int num_threads_;
  const int original_threads_;
};
