#include "rwlock.h"

#include <assert.h>

#include <iostream>

#include "UTestCore.h"
#include "mycoroutine.h"
using namespace std;

namespace {
void WrLockWrap1(MyCoroutine::Schedule &schedule, MyCoroutine::RWLock &rwlock, int &value) {
  MyCoroutine::WrLockGuard lock_guard(rwlock);
  assert(value == 4);
  value++;
  schedule.CoroutineYield();
}
void WrLockWrap2(MyCoroutine::Schedule &schedule, MyCoroutine::RWLock &rwlock, int &value) {
  MyCoroutine::WrLockGuard lock_guard(rwlock);
  assert(value == 5);
  value++;
  schedule.CoroutineYield();
}
void WrLockWrap3(MyCoroutine::Schedule &schedule, MyCoroutine::RWLock &rwlock, int &value) {
  MyCoroutine::WrLockGuard lock_guard(rwlock);
  assert(value == 3);
  value++;
  schedule.CoroutineYield();
}
void RdLockWrap1(MyCoroutine::Schedule &schedule, MyCoroutine::RWLock &rwlock, int &value) {
  MyCoroutine::RdLockGuard lock_guard(rwlock);
  assert(value == 0);
  value++;
  schedule.CoroutineYield();
}
void RdLockWrap2(MyCoroutine::Schedule &schedule, MyCoroutine::RWLock &rwlock, int &value) {
  MyCoroutine::RdLockGuard lock_guard(rwlock);
  assert(value == 1);
  value++;
  schedule.CoroutineYield();
}
void RdLockWrap3(MyCoroutine::Schedule &schedule, MyCoroutine::RWLock &rwlock, int &value) {
  MyCoroutine::RdLockGuard lock_guard(rwlock);
  assert(value == 2);
  value++;
  schedule.CoroutineYield();
}
void WrLockResume4WrThenRd(MyCoroutine::Schedule &schedule, MyCoroutine::RWLock &rwlock, int &value) {
  MyCoroutine::WrLockGuard lock_guard(rwlock);
  assert(value == 0);
  value++;
  schedule.CoroutineYield();
  value++;
}
void RdLockResume4WrThenRd(MyCoroutine::Schedule &schedule, MyCoroutine::RWLock &rwlock, int &value, int &rvalue) {
  MyCoroutine::RdLockGuard lock_guard(rwlock);
  assert(value == 2);
  rvalue++;
}

void WrLockResume4RdThenWr(MyCoroutine::Schedule &schedule, MyCoroutine::RWLock &rwlock, int &value) {
  MyCoroutine::WrLockGuard lock_guard(rwlock);
  value++;
  schedule.CoroutineYield();
  value++;
}
void RdLockResume4RdThenWr(MyCoroutine::Schedule &schedule, MyCoroutine::RWLock &rwlock, int &value) {
  MyCoroutine::RdLockGuard lock_guard(rwlock);
  value++;
  schedule.CoroutineYield();
}
}  // namespace

// 协程读写锁测试用例-WrLockAndRdLockWrap
TEST_CASE(CoRWLock_WrLockAndRdLockWrap) {
  int value = 0;
  MyCoroutine::Schedule schedule(1024);
  MyCoroutine::RWLock rwlock(schedule);
  schedule.CoroutineCreate(RdLockWrap1, std::ref(schedule), std::ref(rwlock), std::ref(value));
  schedule.CoroutineCreate(RdLockWrap2, std::ref(schedule), std::ref(rwlock), std::ref(value));
  schedule.CoroutineCreate(WrLockWrap1, std::ref(schedule), std::ref(rwlock), std::ref(value));
  schedule.CoroutineCreate(WrLockWrap2, std::ref(schedule), std::ref(rwlock), std::ref(value));
  schedule.CoroutineCreate(RdLockWrap3, std::ref(schedule), std::ref(rwlock), std::ref(value));
  schedule.CoroutineCreate(WrLockWrap3, std::ref(schedule), std::ref(rwlock), std::ref(value));
  schedule.Run();
  ASSERT_EQ(value, 6);
}

// 协程读写锁测试用例-Resume-读锁因为写锁而挂起，后面被唤醒
TEST_CASE(CoRWLock_Resume_WrLockThenRdLock) {
  int value = 0;
  int rvalue = 0;
  MyCoroutine::Schedule schedule(1024);
  MyCoroutine::RWLock rwlock(schedule);
  schedule.CoroutineCreate(WrLockResume4WrThenRd, std::ref(schedule), std::ref(rwlock), std::ref(value));
  schedule.CoroutineCreate(RdLockResume4WrThenRd, std::ref(schedule), std::ref(rwlock), std::ref(value),
                           std::ref(rvalue));
  schedule.CoroutineCreate(RdLockResume4WrThenRd, std::ref(schedule), std::ref(rwlock), std::ref(value),
                           std::ref(rvalue));
  schedule.CoroutineResume(0);  // 第一次加写锁成功
  ASSERT_EQ(value, 1);
  schedule.CoroutineResume(1);  // 读锁被挂起
  schedule.CoroutineResume(2);  // 读锁被挂起
  ASSERT_EQ(rvalue, 0);
  schedule.CoroutineResume(0);  // 写锁释放
  ASSERT_EQ(value, 2);
  schedule.CoRWLockResume();  // 唤醒所有的读锁
  ASSERT_EQ(rvalue, 2);
}

// 协程读写锁测试用例-Resume-被写锁挂起的有写锁也有读锁，后面被唤醒
TEST_CASE(CoRWLock_Resume_RdLockThenWrLock) {
  int value = 0;
  MyCoroutine::Schedule schedule(1024);
  MyCoroutine::RWLock rwlock(schedule);
  schedule.CoroutineCreate(WrLockResume4RdThenWr, std::ref(schedule), std::ref(rwlock), std::ref(value));
  schedule.CoroutineCreate(RdLockResume4RdThenWr, std::ref(schedule), std::ref(rwlock), std::ref(value));
  schedule.CoroutineCreate(RdLockResume4RdThenWr, std::ref(schedule), std::ref(rwlock), std::ref(value));
  schedule.CoroutineCreate(WrLockResume4RdThenWr, std::ref(schedule), std::ref(rwlock), std::ref(value));
  schedule.CoroutineCreate(WrLockResume4RdThenWr, std::ref(schedule), std::ref(rwlock), std::ref(value));
  schedule.CoroutineResume(0);  // 第一次加写锁成功
  ASSERT_EQ(value, 1);
  schedule.CoroutineResume(1);  // 读锁被挂起
  schedule.CoroutineResume(2);  // 读锁被挂起
  schedule.CoroutineResume(3);  // 写锁被挂起
  schedule.CoroutineResume(4);  // 写锁被挂起
  ASSERT_EQ(value, 1);
  schedule.CoroutineResume(0);  // 写锁释放
  ASSERT_EQ(value, 2);
  schedule.CoRWLockResume();  // 唤醒2个读锁
  ASSERT_EQ(value, 4);
  schedule.CoroutineResume(1);  // 解读锁
  schedule.CoroutineResume(2);  // 解读锁
  schedule.CoRWLockResume();    // 唤醒1个写锁
  ASSERT_EQ(value, 5);
  schedule.CoroutineResume(3);  // 解写锁
  ASSERT_EQ(value, 6);
  schedule.CoRWLockResume();  // 唤醒最后 1个写锁
  ASSERT_EQ(value, 7);
  schedule.CoroutineResume(4);  // 解写锁
  ASSERT_EQ(value, 8);
}
