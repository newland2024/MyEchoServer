#pragma once

#include "common.h"
#include "mycoroutine.h"

namespace MyCoroutine {
// 协程条件变量类封装
class ConditionVariable {
 public:
  ConditionVariable(Schedule &schedule) : schedule_(schedule) { schedule_.CoCondInit(co_cond_); }
  ~ConditionVariable() { schedule_.CoCondClear(co_cond_); }

  void NotifyOne() { schedule_.CoCondNotifyOne(co_cond_); }
  void NotifyAll() { schedule_.CoCondNotifyAll(co_cond_); }
  void Wait(std::function<bool()> pred) { schedule_.CoCondWait(co_cond_, pred); }

 private:
  CoCond co_cond_;
  Schedule &schedule_;
};
}  // namespace MyCoroutine