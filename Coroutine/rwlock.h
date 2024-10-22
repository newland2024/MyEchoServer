#pragma once

#include "common.h"
#include "mycoroutine.h"

namespace MyCoroutine {
// 协程读写锁
class RWLock {
 public:
  RWLock(Schedule &schedule) : schedule_(schedule) { schedule_.CoRWLockInit(co_rwlock_); }
  ~RWLock() { schedule_.CoRWLockClear(co_rwlock_); }

  void WrLock() { schedule_.CoRWLockWrLock(co_rwlock_); }
  void WrUnLock() { schedule_.CoRWLockWrUnLock(co_rwlock_); }
  void RdLock() { schedule_.CoRWLockRdLock(co_rwlock_); }
  void RdUnLock() { schedule_.CoRWLockRdUnLock(co_rwlock_); }

 private:
  CoRWLock co_rwlock_;
  Schedule &schedule_;
};

// 读锁自动加解锁类封装
class RdLockGuard {
 public:
  RdLockGuard(RWLock &rwlock) : rwlock_(rwlock) { rwlock_.RdLock(); }
  ~RdLockGuard() { rwlock_.RdUnLock(); }

 private:
  RWLock &rwlock_;
};

// 写锁自动加解锁类封装
class WrLockGuard {
 public:
  WrLockGuard(RWLock &rwlock) : rwlock_(rwlock) { rwlock_.WrLock(); }
  ~WrLockGuard() { rwlock_.WrUnLock(); }

 private:
  RWLock &rwlock_;
};
}  // namespace MyCoroutine
