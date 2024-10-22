#pragma once

#include "common.h"
#include "mycoroutine.h"

namespace MyCoroutine {
// 协程CallOnce
class CallOnce {
 public:
  CallOnce(Schedule &schedule) : schedule_(schedule) { schedule_.CoCallOnceInit(co_call_once_); }
  ~CallOnce() { schedule_.CoCallOnceClear(co_call_once_); }

  template <typename Function, typename... Args>
  void Do(Function &&func, Args &&...args) {
    schedule_.CoCallOnceDo(co_call_once_, std::forward<Function>(func), std::forward<Args>(args)...);
  }

 private:
  CoCallOnce co_call_once_;
  Schedule &schedule_;
};
}  // namespace MyCoroutine
