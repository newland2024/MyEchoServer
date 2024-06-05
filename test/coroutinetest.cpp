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

TEST_CASE(Coroutine_Mutex) {
  Schedule schedule;
  ScheduleInit(schedule, 1024);
  int sum = 0;
  CoMutex mutex;
  CoMutexInit(schedule, mutex);
  ASSERT_FALSE(ScheduleRunning(schedule));
  int id1 = CoroutineCreate(schedule, MutexTest1, &schedule, &mutex, &sum);
  ASSERT_EQ(id1, 0);
  int id2 = CoroutineCreate(schedule, MutexTest2, &schedule, &mutex, &sum);
  ASSERT_EQ(id2, 1);
  bool round = false;
  while (ScheduleRunning(schedule)) {
    if (round) {
      CoroutineResumeById(schedule, id1);
      CoroutineResumeById(schedule, id2);
      round = false;
    } else {
      CoroutineResumeById(schedule, id2);
      CoroutineResumeById(schedule, id1);
      round = true;
    }
    ScheduleRun(schedule);
  }
  ScheduleClean(schedule);
  std::cout << "sum = " << sum << std::endl;
}

// 需要测试循环唤醒Lock函数，但是获取不到锁。Mutex还需要专门写一个验收列表，来一个一个测试！