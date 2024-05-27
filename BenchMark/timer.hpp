#pragma once

#include <assert.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include <queue>
#include <unordered_set>

namespace BenchMark {
typedef struct TimerData {
  friend bool operator<(const TimerData& left, const TimerData& right) {
    return left.abs_time_ms_ > right.abs_time_ms_;
  }
  uint64_t id_;
  int64_t abs_time_ms_{0};
  std::function<void()> call_back_{nullptr};
} TimerData;

class Timer {
 public:
  template <typename Function, typename... Args>
  uint64_t Register(int64_t time_out_ms, Function&& f, Args&&... args) {
    alloc_id_++;
    TimerData timer_data;
    timer_data.id_ = alloc_id_;
    timer_data.abs_time_ms_ = GetCurrentTimeMs() + time_out_ms;
    std::function<void()> call_back = std::bind(std::forward<Function>(f), std::forward<Args>(args)...);
    timer_data.call_back_ = call_back;
    timers_.push(timer_data);
    timer_ids_.insert(timer_data.id_);
    return alloc_id_;
  }
  void Cancel(uint64_t id) {
    assert(timer_ids_.find(id) != timer_ids_.end());  // 取消的定时器，必须是存在的
    cancel_ids_.insert(id);  // 这里只是做一下记录
  }
  bool GetLastTimer(TimerData& timer_data) {
    while (not timers_.empty()) {
      timer_data = timers_.top();
      timers_.pop();
      if (cancel_ids_.find(timer_data.id_) != cancel_ids_.end()) {  // 被取消的定时器不执行，直接删除
        cancel_ids_.erase(timer_data.id_);
        timer_ids_.erase(timer_data.id_);
        continue;
      }
      return true;
    }
    return false;
  }
  int64_t TimeOutMs(TimerData& timer_data) {
    int64_t temp = timer_data.abs_time_ms_ + 1 - GetCurrentTimeMs();  // 多1ms，确保后续的定时器必定能超时
    if (temp > 0) {
      return temp;
    }
    return 0;
  }
  void Run(TimerData& timer_data) {
    if (not isExpire(timer_data)) {  // 没有过期则重新塞入队列中去重新排队
      timers_.push(timer_data);
      return;
    }
    int64_t timer_id = timer_data.id_;
    assert(timer_ids_.find(timer_id) != timer_ids_.end());  // 要运行的定时器，必须是存在的
    timer_ids_.erase(timer_id);
    if (cancel_ids_.find(timer_id) != cancel_ids_.end()) {  // 被取消的定时器不执行，直接删除
      cancel_ids_.erase(timer_id);
      return;
    }
    timer_data.call_back_();
  }
  int64_t GetCurrentTimeMs() {
    struct timeval current;
    gettimeofday(&current, NULL);
    return current.tv_sec * 1000 + current.tv_usec / 1000;
  }

 private:
  bool isExpire(TimerData& timer_data) { return GetCurrentTimeMs() >= timer_data.abs_time_ms_; }

 private:
  uint64_t alloc_id_{0};
  std::unordered_set<uint64_t> timer_ids_;
  std::unordered_set<uint64_t> cancel_ids_;
  std::priority_queue<TimerData> timers_;
};
}  // namespace BenchMark
