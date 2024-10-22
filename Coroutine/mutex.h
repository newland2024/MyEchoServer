#pragma once

#include "common.h"
#include "mycoroutine.h"

namespace MyCoroutine {
// 协程互斥锁
class Mutex {
 public:
  Mutex(Schedule &schedule) : schedule_(schedule) { schedule_.CoMutexInit(mutex_); }
  ~Mutex() { schedule_.CoMutexClear(mutex_); }

  void Lock() { schedule_.CoMutexLock(mutex_); }
  void UnLock() { schedule_.CoMutexUnLock(mutex_); }
  bool TryLock() { return schedule_.CoMutexTryLock(mutex_); }

 private:
  CoMutex mutex_;
  Schedule &schedule_;
};

// 自动加解锁类封装
class LockGuard {
 public:
  LockGuard(Mutex &mutex) : mutex_(mutex) { mutex_.Lock(); }
  ~LockGuard() { mutex_.UnLock(); }

 private:
  Mutex &mutex_;
};
}  // namespace MyCoroutine
