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