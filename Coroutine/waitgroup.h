#pragma once

#include "common.h"
#include "mycoroutine.h"

namespace MyCoroutine {
class WaitGroup {
 public:
  WaitGroup(Schedule &schedule) : schedule_(schedule) { bid_ = schedule_.BatchCreate(); }
  template <typename Function, typename... Args>
  bool Add(Function &&func, Args &&...args) {
    return schedule_.BatchAdd(bid_, std::forward<Function>(func), std::forward<Args>(args)...);
  }
  void Wait() { schedule_.BatchRun(bid_); }

 private:
  int bid_{kInvalidBid};  // Batch的id
  Schedule &schedule_;
};
}  // namespace MyCoroutine