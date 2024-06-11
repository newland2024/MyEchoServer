#include "coroutine.h"
#include "unittestcore.hpp"

using namespace MyCoroutine;

void print(Schedule* schedule) {
  std::cout << "print begin" << std::endl;
  CoroutineYield(*schedule);
  std::cout << "print end" << std::endl;
}

TEST_CASE(Schedule_ALL) {
  Schedule schedule;
  ScheduleInit(schedule, 1024);
  ASSERT_FALSE(ScheduleRunning(schedule));
  ScheduleClean(schedule);
}

TEST_CASE(Coroutine_Success) {
  Schedule schedule;
  ScheduleInit(schedule, 1024);
  ASSERT_FALSE(ScheduleRunning(schedule));
  int id = CoroutineCreate(schedule, print, &schedule);
  ASSERT_EQ(id, 0);
  while (ScheduleRunning(schedule)) {
    ASSERT_EQ(CoroutineResume(schedule), Success);
    ASSERT_EQ(CoroutineResumeById(schedule, id), Success);
  }
  ScheduleClean(schedule);
}

TEST_CASE(Coroutine_Failure) {
  Schedule schedule;
  ScheduleInit(schedule, 1);
  ASSERT_FALSE(ScheduleRunning(schedule));
  int id = CoroutineCreate(schedule, print, &schedule);
  ASSERT_EQ(id, 0);
  id = CoroutineCreate(schedule, print, &schedule);
  ASSERT_EQ(id, -1);
  ASSERT_EQ(CoroutineResume(schedule), Success);
  ASSERT_EQ(CoroutineResume(schedule), Success);
  ASSERT_EQ(CoroutineResume(schedule), NotRunnable);
  ASSERT_EQ(CoroutineResumeById(schedule, 0), NotRunnable);
  ScheduleClean(schedule);
}

void MutexSingleCoroutine(Schedule& schedule, CoMutex& mutex, int& sum) {
  CoMutexLock(schedule, mutex);
  sum = 666;
  CoMutexUnLock(schedule, mutex);
}

TEST_CASE(Mutex_SingleCoroutine) {
  Schedule schedule;
  ScheduleInit(schedule, 1024);
  int sum = 0;
  CoMutex mutex;
  CoMutexInit(schedule, mutex);
  ASSERT_FALSE(ScheduleRunning(schedule));
  int id = CoroutineCreate(schedule, MutexSingleCoroutine, std::ref(schedule), std::ref(mutex), std::ref(sum));
  ASSERT_EQ(id, 0);
  while (ScheduleRunning(schedule)) {
    CoroutineResume(schedule);
  }
  CoMutexClear(schedule, mutex);
  ScheduleClean(schedule);
  ASSERT_EQ(sum, 666);
  std::cout << "sum = " << sum << std::endl;
}

void MutexMultiCoroutine0(Schedule& schedule, CoMutex& mutex, int& sum) {
  CoMutexLock(schedule, mutex);
  std::cout << "MutexMultiCoroutine0 lock" << std::endl;
  if (sum == -1) {
    sum = 0;
    std::cout << "MutexMultiCoroutine0 set sum 0, yield" << std::endl;
  }
  CoroutineYield(schedule);
  std::cout << "MutexMultiCoroutine0 unlock" << std::endl;
  CoMutexUnLock(schedule, mutex);
}

void MutexMultiCoroutine1(Schedule& schedule, CoMutex& mutex, int& sum) {
  CoMutexLock(schedule, mutex);
  std::cout << "MutexMultiCoroutine1 lock" << std::endl;
  if (sum == 2) {
    sum = 1;
    std::cout << "MutexMultiCoroutine1 set sum 1, yield" << std::endl;
  }
  CoroutineYield(schedule);
  std::cout << "MutexMultiCoroutine1 unlock" << std::endl;
  CoMutexUnLock(schedule, mutex);
}

void MutexMultiCoroutine2(Schedule& schedule, CoMutex& mutex, int& sum) {
  CoMutexLock(schedule, mutex);
  std::cout << "MutexMultiCoroutine2 lock" << std::endl;
  if (sum == 0) {
    sum = 2;
    std::cout << "MutexMultiCoroutine2 set sum 2, yield" << std::endl;
  }
  CoroutineYield(schedule);
  std::cout << "MutexMultiCoroutine2 unlock" << std::endl;
  CoMutexUnLock(schedule, mutex);
}

TEST_CASE(Mutex_Priority) {
  Schedule schedule;
  ScheduleInit(schedule, 1024);
  int sum = -1;
  CoMutex mutex;
  CoMutexInit(schedule, mutex);
  ASSERT_FALSE(ScheduleRunning(schedule));
  int id0 = CoroutineCreate(schedule, MutexMultiCoroutine0, std::ref(schedule), std::ref(mutex), std::ref(sum));
  ASSERT_EQ(id0, 0);
  int id1 = CoroutineCreate(schedule, MutexMultiCoroutine1, std::ref(schedule), std::ref(mutex), std::ref(sum));
  ASSERT_EQ(id1, 1);
  int id2 = CoroutineCreate(schedule, MutexMultiCoroutine2, std::ref(schedule), std::ref(mutex), std::ref(sum));
  ASSERT_EQ(id2, 2);

  CoroutineResumeById(schedule, id0);
  CoroutineResumeById(schedule, id2);
  CoroutineResumeById(schedule, id1);
  CoroutineResumeById(schedule, id0);

  while (ScheduleRunning(schedule)) {
    ScheduleRun(schedule);
    CoroutineResumeAll(schedule);
  }
  CoMutexClear(schedule, mutex);
  ScheduleClean(schedule);
  ASSERT_EQ(sum, 1);
  std::cout << "sum = " << sum << std::endl;
}

void MutexMulti(Schedule& schedule, CoMutex& mutex, int& sum) {
  for (int i = 0; i < 1000; i++) {
    CoMutexLock(schedule, mutex);
    sum++;
    CoroutineYield(schedule);
    CoMutexUnLock(schedule, mutex);
    CoroutineYield(schedule);
  }
}

TEST_CASE(Mutex_Multi) {
  Schedule schedule;
  ScheduleInit(schedule, 1024);
  int sum = 0;
  CoMutex mutex;
  CoMutexInit(schedule, mutex);
  ASSERT_FALSE(ScheduleRunning(schedule));
  for (int i = 0; i < 100; i++) {
    CoroutineCreate(schedule, MutexMulti, std::ref(schedule), std::ref(mutex), std::ref(sum));
  }
  while (ScheduleRunning(schedule)) {
    ScheduleRun(schedule);
    CoroutineResumeAll(schedule);
  }
  CoMutexClear(schedule, mutex);
  ScheduleClean(schedule);
  ASSERT_EQ(sum, 100000);
  std::cout << "sum = " << sum << std::endl;
}

void MutexFirstLock(Schedule& schedule, CoMutex& mutex, int& sum) {
  CoMutexLock(schedule, mutex);
  sum = 1;
  CoroutineYield(schedule);
  sum = 3;
  CoMutexUnLock(schedule, mutex);
}

void MutexFailureFirstLock(Schedule& schedule, CoMutex& mutex, int& sum) {
  CoMutexLock(schedule, mutex);
  sum = 2;
  CoroutineYield(schedule);
  sum = 4;
  CoMutexUnLock(schedule, mutex);
}

TEST_CASE(Mutex_FailureFirstLock) {
  Schedule schedule;
  ScheduleInit(schedule, 1024);
  int sum = 0;
  CoMutex mutex;
  CoMutexInit(schedule, mutex);
  ASSERT_FALSE(ScheduleRunning(schedule));
  int id = CoroutineCreate(schedule, MutexFirstLock, std::ref(schedule), std::ref(mutex), std::ref(sum));
  int id1 = CoroutineCreate(schedule, MutexFailureFirstLock, std::ref(schedule), std::ref(mutex), std::ref(sum));
  CoroutineResumeById(schedule, id);  // 锁定成功
  CoroutineResumeById(schedule, id1);  // 锁定失败
  ASSERT_EQ(sum, 1);  // 值为1
  CoroutineResumeById(schedule, id);
  ASSERT_EQ(sum, 3);  // 值被修改成3
  CoroutineResumeById(schedule, id1);
  ASSERT_EQ(sum, 2);  // 值被修改成2
  CoroutineResumeById(schedule, id1);
  ASSERT_EQ(sum, 4);  // 值被修改成2
  CoMutexClear(schedule, mutex);
  ScheduleClean(schedule);
}

TEST_CASE(Mutex_AllLockFailure) {
  Schedule schedule;
  ScheduleInit(schedule, 1024);
  int sum = 0;
  CoMutex mutex;
  CoMutexInit(schedule, mutex);
  ASSERT_FALSE(ScheduleRunning(schedule));
  int id = CoroutineCreate(schedule, MutexFirstLock, std::ref(schedule), std::ref(mutex), std::ref(sum));
  int id1 = CoroutineCreate(schedule, MutexFailureFirstLock, std::ref(schedule), std::ref(mutex), std::ref(sum));
  CoroutineResumeById(schedule, id);  // 锁定成功
  int count = 0;
  while (count++ <= 10000) {  // 一直锁定失败，sum的值一直为1
    CoroutineResumeById(schedule, id1);  // 锁定失败
    ASSERT_EQ(sum, 1);
  }
  CoMutexClear(schedule, mutex);
  ScheduleClean(schedule);
}

void MutexLockReEntry(Schedule& schedule, CoMutex& mutex, int& sum) {
  CoMutexLock(schedule, mutex);
  // CoMutexLock(schedule, mutex);  // 同一个从协程重入，触发断言
  sum = 2;
  CoMutexUnLock(schedule, mutex);
}

TEST_CASE(Mutex_LockReEntry) {
  Schedule schedule;
  ScheduleInit(schedule, 1024);
  int sum = 0;
  CoMutex mutex;
  CoMutexInit(schedule, mutex);
  ASSERT_FALSE(ScheduleRunning(schedule));
  int id = CoroutineCreate(schedule, MutexLockReEntry, std::ref(schedule), std::ref(mutex), std::ref(sum));
  CoroutineResumeById(schedule, id);  // 锁定成功
  CoMutexClear(schedule, mutex);
  ScheduleClean(schedule);
}

void CondWaitPredTrue(Schedule& schedule, CoCond& cond, std::list<int>& q, int& value) {
  CoCondWait(schedule, cond, [&q]() { return q.size() > 0; });
  value = q.front();
  q.pop_front();
  std::cout << "cond_wait q.front() = " << value << std::endl;
}

TEST_CASE(Cond_Wait_PredTrue) {
  Schedule schedule;
  ScheduleInit(schedule, 1024);
  CoCond cond;
  std::list<int> q;
  int value = 0;
  q.push_back(200);
  CoCondInit(schedule, cond);
  ASSERT_FALSE(ScheduleRunning(schedule));
  int id =
      CoroutineCreate(schedule, CondWaitPredTrue, std::ref(schedule), std::ref(cond), std::ref(q), std::ref(value));
  CoroutineResumeById(schedule, id);  // 等待条件变量通知
  CoCondClear(schedule, cond);
  ScheduleClean(schedule);
  ASSERT_EQ(value, 200);
}

void CondWaitValid(Schedule& schedule, CoCond& cond, std::list<int>& q, int& value, int& awake_count) {
  CoCondWait(schedule, cond, [&q]() { return q.size() > 0; });
  value += q.front();
  awake_count++;
  std::cout << "cond_wait q.front() = " << q.front() << ", awake_count = " << awake_count << std::endl;
  q.pop_front();
}

void CondWaitValidNotifyOne(Schedule& schedule, CoCond& cond, std::list<int>& q) {
  int value = 100;
  q.push_back(value);
  CoCondNotifyOne(schedule, cond);
  std::cout << "cond_notify_one q insert value = " << value << std::endl;
}

void CondWaitValidNotifyAll(Schedule& schedule, CoCond& cond, std::list<int>& q) {
  int value = 100;
  q.push_back(value);
  q.push_back(value);
  CoCondNotifyAll(schedule, cond);
  std::cout << "cond_notify_all q insert value = " << value << std::endl;
}

TEST_CASE(Cond_Wait_Valid) {
  Schedule schedule;
  ScheduleInit(schedule, 1024);
  CoCond cond;
  std::list<int> q;
  int value = 0;
  int awake_count = 0;
  CoCondInit(schedule, cond);
  ASSERT_FALSE(ScheduleRunning(schedule));
  int id = CoroutineCreate(schedule, CondWaitValid, std::ref(schedule), std::ref(cond), std::ref(q), std::ref(value),
                           std::ref(awake_count));
  int id1 = CoroutineCreate(schedule, CondWaitValid, std::ref(schedule), std::ref(cond), std::ref(q), std::ref(value),
                            std::ref(awake_count));
  int id2 = CoroutineCreate(schedule, CondWaitValidNotifyOne, std::ref(schedule), std::ref(cond), std::ref(q));
  CoroutineResumeById(schedule, id);  // 等待条件变量通知
  CoroutineResumeById(schedule, id1);  // 等待条件变量通知
  CoroutineResumeById(schedule, id2);  // 触发条件变量的通知

  ScheduleRun(schedule);
  CoCondClear(schedule, cond);
  ScheduleClean(schedule);
  ASSERT_EQ(value, 100);
  ASSERT_EQ(awake_count, 1);
}

void CondWaitInValid(Schedule& schedule, CoCond& cond, std::list<int>& q, int& value) {
  std::cout << "CondWaitInValid call " << std::endl;
  CoCondWait(schedule, cond, [&q]() { return q.size() > 0; });
  value = q.front();
  q.pop_front();
  std::cout << "cond_wait q.front() = " << value << std::endl;
}

TEST_CASE(Cond_Wait_InValid) {
  Schedule schedule;
  ScheduleInit(schedule, 1024);
  CoCond cond;
  std::list<int> q;
  int value = 0;
  CoCondInit(schedule, cond);
  ASSERT_FALSE(ScheduleRunning(schedule));
  int id = CoroutineCreate(schedule, CondWaitInValid, std::ref(schedule), std::ref(cond), std::ref(q), std::ref(value));
  CoroutineResumeById(schedule, id);  // 等待条件变量通知
  CoCondNotifyOne(schedule, cond);
  int count = 0;
  while (count++ <= 100) {
    ScheduleRun(schedule);  // 连续唤醒100次，因为条件不成立，则CoCondWait无法返回
  }
  CoCondClear(schedule, cond);
  ScheduleClean(schedule);
  ASSERT_EQ(value, 0);
}

TEST_CASE(Cond_Wait_Notify_All) {
  Schedule schedule;
  ScheduleInit(schedule, 1024);
  CoCond cond;
  std::list<int> q;
  int value = 0;
  int awake_count = 0;
  CoCondInit(schedule, cond);
  ASSERT_FALSE(ScheduleRunning(schedule));
  int id = CoroutineCreate(schedule, CondWaitValid, std::ref(schedule), std::ref(cond), std::ref(q), std::ref(value),
                           std::ref(awake_count));
  int id1 = CoroutineCreate(schedule, CondWaitValid, std::ref(schedule), std::ref(cond), std::ref(q), std::ref(value),
                            std::ref(awake_count));
  int id2 = CoroutineCreate(schedule, CondWaitValidNotifyAll, std::ref(schedule), std::ref(cond), std::ref(q));
  CoroutineResumeById(schedule, id);  // 等待条件变量通知
  CoroutineResumeById(schedule, id1);  // 等待条件变量通知
  CoroutineResumeById(schedule, id2);  // 触发条件变量的通知

  ScheduleRun(schedule);
  CoCondClear(schedule, cond);
  ScheduleClean(schedule);
  ASSERT_EQ(value, 200);
  ASSERT_EQ(awake_count, 2);
}