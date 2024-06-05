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

void MutexTest1(Schedule* schedule, CoMutex* mutex, int* sum) {
  for (int i = 0; i < 10000; i++) {
    CoMutexLock(*schedule, *mutex);
    (*sum)++;
    CoroutineYield(*schedule);
    CoMutexUnLock(*schedule, *mutex);
    CoroutineYield(*schedule);
  }
}

void MutexTest2(Schedule* schedule, CoMutex* mutex, int* sum) {
  for (int i = 0; i < 10000; i++) {
    CoMutexLock(*schedule, *mutex);
    (*sum) += 2;
    CoroutineYield(*schedule);
    CoMutexUnLock(*schedule, *mutex);
    CoroutineYield(*schedule);
  }
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
  CoMutexLock(schedule, mutex, true);
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
  CoMutexLock(schedule, mutex, true);
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
  CoMutexLock(schedule, mutex, true);
  std::cout << "MutexMultiCoroutine2 lock" << std::endl;
  if (sum == 0) {
    sum = 2;
    std::cout << "MutexMultiCoroutine2 set sum 2, yield" << std::endl;
  }
  CoroutineYield(schedule);
  std::cout << "MutexMultiCoroutine2 unlock" << std::endl;
  CoMutexUnLock(schedule, mutex);
}

TEST_CASE(Mutex_MultiCoroutine) {
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

// TEST_CASE(Coroutine_Mutex) {
//   Schedule schedule;
//   ScheduleInit(schedule, 1024);
//   int sum = 0;
//   CoMutex mutex;
//   CoMutexInit(schedule, mutex);
//   ASSERT_FALSE(ScheduleRunning(schedule));
//   int id1 = CoroutineCreate(schedule, MutexTest1, &schedule, &mutex, &sum);
//   ASSERT_EQ(id1, 0);
//   int id2 = CoroutineCreate(schedule, MutexTest2, &schedule, &mutex, &sum);
//   ASSERT_EQ(id2, 1);
//   bool round = false;
//   while (ScheduleRunning(schedule)) {
//     if (round) {
//       CoroutineResumeById(schedule, id1);
//       CoroutineResumeById(schedule, id2);
//       round = false;
//     } else {
//       CoroutineResumeById(schedule, id2);
//       CoroutineResumeById(schedule, id1);
//       round = true;
//     }
//     ScheduleRun(schedule);
//   }
//   ScheduleClean(schedule);
//   std::cout << "sum = " << sum << std::endl;
// }

// 需要测试循环唤醒Lock函数，但是获取不到锁。Mutex还需要专门写一个验收列表，来一个一个测试！