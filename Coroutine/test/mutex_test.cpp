#include "mutex.h"

#include <assert.h>

#include <iostream>

#include "UTestCore.h"
#include "mycoroutine.h"
using namespace std;

namespace {
void Mutex1(MyCoroutine::Schedule &schedule, MyCoroutine::CoMutex &co_mutex, int &value) {
  schedule.CoMutexLock(co_mutex);
  assert(value == 0);
  schedule.CoroutineYield();
  value++;
  schedule.CoMutexUnLock(co_mutex);
}
void Mutex2(MyCoroutine::Schedule &schedule, MyCoroutine::CoMutex &co_mutex, int &value) {
  schedule.CoMutexLock(co_mutex);
  assert(value == 1);
  schedule.CoroutineYield();
  value++;
  schedule.CoMutexUnLock(co_mutex);
}
void Mutex3(MyCoroutine::Schedule &schedule, MyCoroutine::CoMutex &co_mutex, int &value) {
  schedule.CoMutexLock(co_mutex);
  assert(value == 2);
  schedule.CoroutineYield();
  value++;
  schedule.CoMutexUnLock(co_mutex);
}

void MutexWrap1(MyCoroutine::Schedule &schedule, MyCoroutine::Mutex &mutex, int &value) {
  MyCoroutine::LockGuard lock_guard(mutex);
  assert(value == 0);
  schedule.CoroutineYield();
  value++;
}
void MutexWrap2(MyCoroutine::Schedule &schedule, MyCoroutine::Mutex &mutex, int &value) {
  MyCoroutine::LockGuard lock_guard(mutex);
  assert(value == 1);
  schedule.CoroutineYield();
  value++;
}
void MutexWrap3(MyCoroutine::Schedule &schedule, MyCoroutine::Mutex &mutex, int &value) {
  MyCoroutine::LockGuard lock_guard(mutex);
  assert(value == 2);
  schedule.CoroutineYield();
  value++;
}

void MutexTryLock1(MyCoroutine::Schedule &schedule, MyCoroutine::CoMutex &co_mutex, int &value) {
  bool lock = schedule.CoMutexTryLock(co_mutex);
  assert(value == 0 && lock);
  schedule.CoroutineYield();
  value++;
  schedule.CoMutexUnLock(co_mutex);
}

void MutexTryLock2(MyCoroutine::Schedule &schedule, MyCoroutine::CoMutex &co_mutex, int &value) {
  bool lock = schedule.CoMutexTryLock(co_mutex);
  assert(not lock);
  if (not lock) {
    return;
  }
  assert(0);
}
}  // namespace

// 协程互斥量测试用例-LockAndUnLock
TEST_CASE(CoMutex_LockAndUnLock) {
  int value = 0;
  MyCoroutine::CoMutex co_mutex;
  MyCoroutine::Schedule schedule(1024);
  schedule.CoMutexInit(co_mutex);
  schedule.CoroutineCreate(Mutex1, std::ref(schedule), std::ref(co_mutex), std::ref(value));
  schedule.CoroutineCreate(Mutex2, std::ref(schedule), std::ref(co_mutex), std::ref(value));
  schedule.CoroutineCreate(Mutex3, std::ref(schedule), std::ref(co_mutex), std::ref(value));
  schedule.Run();
  schedule.CoMutexClear(co_mutex);
  ASSERT_EQ(value, 3);
}

// 协程互斥量测试用例-LockAndUnLockWrap
TEST_CASE(CoMutex_LockAndUnLockWrap) {
  int value = 0;
  MyCoroutine::Schedule schedule(1024);
  MyCoroutine::Mutex mutex(schedule);
  schedule.CoroutineCreate(MutexWrap1, std::ref(schedule), std::ref(mutex), std::ref(value));
  schedule.CoroutineCreate(MutexWrap2, std::ref(schedule), std::ref(mutex), std::ref(value));
  schedule.CoroutineCreate(MutexWrap3, std::ref(schedule), std::ref(mutex), std::ref(value));
  schedule.Run();
  ASSERT_EQ(value, 3);
}

// 协程互斥量测试用例-TryLock
TEST_CASE(CoMutex_TryLock) {
  int value = 0;
  MyCoroutine::CoMutex co_mutex;
  MyCoroutine::Schedule schedule(1024);
  schedule.CoMutexInit(co_mutex);
  schedule.CoroutineCreate(MutexTryLock1, std::ref(schedule), std::ref(co_mutex), std::ref(value));
  schedule.CoroutineCreate(MutexTryLock2, std::ref(schedule), std::ref(co_mutex), std::ref(value));
  schedule.Run();
  schedule.CoMutexClear(co_mutex);
  ASSERT_EQ(value, 1);
}

// 协程互斥量测试用例-MutexResume
TEST_CASE(CoMutex_MutexResume) {
  int value = 0;
  MyCoroutine::CoMutex co_mutex;
  MyCoroutine::Schedule schedule(1024);
  schedule.CoMutexInit(co_mutex);
  schedule.CoroutineCreate(Mutex1, std::ref(schedule), std::ref(co_mutex), std::ref(value));
  schedule.CoroutineCreate(Mutex2, std::ref(schedule), std::ref(co_mutex), std::ref(value));
  schedule.CoroutineCreate(Mutex3, std::ref(schedule), std::ref(co_mutex), std::ref(value));
  schedule.CoroutineResume(0);
  schedule.CoroutineResume(1);
  schedule.CoroutineResume(2);
  schedule.CoMutexResume();  // cid = 0获取到锁

  schedule.CoroutineResume(0);  // cid = 0释放锁
  schedule.CoMutexResume();     // cid = 1获取到锁

  schedule.CoroutineResume(1);  // cid = 1释放锁
  schedule.CoMutexResume();     // cid = 2获取到锁
  schedule.CoroutineResume(2);  // cid = 2释放锁

  schedule.CoMutexClear(co_mutex);
  ASSERT_EQ(value, 3);
}