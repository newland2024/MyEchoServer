#pragma once

#include <assert.h>
#include <sys/time.h>
#include <time.h>

#include <cstdint>
#include <functional>
#include <queue>
#include <unordered_set>
#include <utility>

namespace EventDriven {
typedef struct TimerData {
  friend bool operator<(const TimerData &left, const TimerData &right) { return left.abs_time_ms > right.abs_time_ms; }
  uint64_t id;                    // 定时器唯一id
  int64_t abs_time_ms;            // 超时时间
  std::function<void()> handler;  // 定时器超时处理函数
} TimerData;

class Timer {
 public:
  template <typename Function, typename... Args>
  uint64_t Register(int64_t time_out_ms, Function &&handler, Args &&...args) {
    alloc_id_++;
    TimerData timer_data;
    timer_data.id = alloc_id_;
    timer_data.abs_time_ms = GetCurrentTimeMs() + time_out_ms;
    timer_data.handler = std::bind(std::forward<Function>(handler), std::forward<Args>(args)...);
    timers_.push(timer_data);
    timer_ids_.insert(timer_data.id);
    return timer_data.id;
  }

  void Cancel(uint64_t id) {
    assert(timer_ids_.find(id) != timer_ids_.end());  // 取消的定时器，必须是存在的
    cancel_ids_.insert(id);                           // 这里只是做一下记录，延后删除策略
  }
  bool GetLastTimer(TimerData &timer_data) {
    while (not timers_.empty()) {
      timer_data = timers_.top();
      timers_.pop();
      if (cancel_ids_.find(timer_data.id) != cancel_ids_.end()) {  // 被取消的定时器不执行，直接删除
        cancel_ids_.erase(timer_data.id);
        timer_ids_.erase(timer_data.id);
        continue;
      }
      return true;
    }
    assert(timer_ids_.size() == 0);
    assert(cancel_ids_.size() == 0);
    return false;
  }
  int64_t TimeOutMs(TimerData &timer_data) {
    int64_t temp = timer_data.abs_time_ms + 1 - GetCurrentTimeMs();  // 多1ms，确保后续的定时器必定能超时
    if (temp > 0) {
      return temp;
    }
    return 0;
  }
  void Run(TimerData &timer_data) {
    if (not isExpire(timer_data)) {  // 没有过期则重新塞入队列中去重新排队
      timers_.push(timer_data);
      return;
    }
    assert(timer_ids_.find(timer_data.id) != timer_ids_.end());  // 要运行的定时器，必须是存在的
    timer_ids_.erase(timer_data.id);
    if (cancel_ids_.find(timer_data.id) != cancel_ids_.end()) {  // 被取消的定时器不执行，直接删除
      cancel_ids_.erase(timer_data.id);
      return;
    }
    timer_data.handler();  // 执行定时器的超时处理函数
  }
  int64_t GetCurrentTimeMs() {
    struct timeval current;
    gettimeofday(&current, nullptr);
    return current.tv_sec * 1000 + current.tv_usec / 1000;
  }

 private:
  bool isExpire(TimerData &timer_data) { return GetCurrentTimeMs() >= timer_data.abs_time_ms; }

 private:
  uint64_t alloc_id_{0};                     // 用于定时器id分配
  std::unordered_set<uint64_t> timer_ids_;   // 用于分配的定时器id
  std::unordered_set<uint64_t> cancel_ids_;  // 用于记录被取消的定时器id
  std::priority_queue<TimerData> timers_;    // 定时器优先队列
};
}  // namespace EventDriven