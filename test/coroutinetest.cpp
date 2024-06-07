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
  ScheduleClean(schedule);
}